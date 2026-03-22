/**
 * @file coin_quick_item.cpp
 * @brief CoinQuickItem implementation: FBO rendering, mouse navigation, camera state.
 */

#include <opengeolab/app/coin_quick_item.hpp>

#include <opengeolab/base/module_service_interface.hpp>
#include <opengeolab/render/camera_state.hpp>
#include <opengeolab/render/navigation_controller.hpp>
#include <opengeolab/render/render_module.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <Inventor/SbViewVolume.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/projectors/SbSphereSheetProjector.h>

#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QWheelEvent>

#include <nlohmann/json.hpp>

#include <array>
#include <utility>

namespace {

void reorient_camera(SoCamera* camera, const SbRotation& rotation,
                     const SbVec3f& rotation_center) {
    SbVec3f offset_cam;
    camera->orientation.getValue().inverse().multVec(
        camera->position.getValue() - rotation_center, offset_cam);
    camera->orientation = rotation * camera->orientation.getValue();
    SbVec3f new_offset;
    camera->orientation.getValue().multVec(offset_cam, new_offset);
    camera->position = rotation_center + new_offset;
}

} // namespace

namespace OpenGeoLab::App {

class CoinQuickItemRenderer : public QQuickFramebufferObject::Renderer {
public:
    explicit CoinQuickItemRenderer(std::shared_ptr<Render::SceneManager> mgr)
        : scene_manager_(std::move(mgr)) {}

    ~CoinQuickItemRenderer() override { delete render_action_; }

    auto createFramebufferObject(const QSize& size) -> QOpenGLFramebufferObject* override {
        fbo_size_ = size;
        viewport_region_ = SbViewportRegion(size.width(), size.height());
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    void render() override {
        if(!scene_manager_ || !scene_manager_->root_node()) {
            return;
        }

        auto* context = QOpenGLContext::currentContext();
        if(context == nullptr) {
            return;
        }

        auto* gl = context->functions();
        gl->glClearColor(0.15F, 0.15F, 0.15F, 1.F);
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl->glEnable(GL_DEPTH_TEST);

        if(render_action_ == nullptr) {
            render_action_ = new SoGLRenderAction(viewport_region_);
        }
        render_action_->setViewportRegion(viewport_region_);
        render_action_->apply(scene_manager_->root_node());
    }

    void synchronize(QQuickFramebufferObject* /*item*/) override {
        // Could sync state from item to renderer here if needed.
    }

private:
    std::shared_ptr<Render::SceneManager> scene_manager_;
    SoGLRenderAction* render_action_ = nullptr;
    SbViewportRegion viewport_region_;
    QSize fbo_size_;
};

CoinQuickItem::CoinQuickItem(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    try {
        auto& factory = Kangaroo::Util::PluginComponentFactory::instance();
        auto module = factory.getSharedInstance<OpenGeoLab::Base::IModuleService>("render");
        auto render_module = std::dynamic_pointer_cast<Render::RenderModule>(module);
        if(render_module) {
            scene_manager_ = render_module->scene_manager();
        }
    } catch(const std::exception& ex) {
        qWarning("CoinQuickItem: failed to resolve render module: %s", ex.what());
    }

    if(scene_manager_) {
        scene_manager_->add_box(2.F, 2.F, 2.F);
    }

    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptTouchEvents(true);
    projector_ = std::make_unique<SbSphereSheetProjector>(
        SbSphere(SbVec3f(0.F, 0.F, 0.F), 0.8F), TRUE);

    // Fixed ortho view volume (FreeCAD pattern) — provides consistent coordinate space
    SbViewVolume ortho_vv;
    ortho_vv.ortho(-1, 1, -1, 1, -1, 1);
    projector_->setViewVolume(ortho_vv);
}

CoinQuickItem::~CoinQuickItem() = default;

auto CoinQuickItem::createRenderer() const -> QQuickFramebufferObject::Renderer* {
    return new CoinQuickItemRenderer(scene_manager_);
}

void CoinQuickItem::viewAll() {
    if(!scene_manager_) {
        return;
    }
    scene_manager_->view_all(static_cast<int>(width()), static_cast<int>(height()));
    update();
}

void CoinQuickItem::requestUpdate() { update(); }

void CoinQuickItem::restoreCameraState(const QString& json) {
    if(!scene_manager_) {
        return;
    }
    const auto state = Render::CameraState::from_json(nlohmann::json::parse(json.toStdString()));
    scene_manager_->restore_camera_state(state);
    update();
}

auto CoinQuickItem::cameraStateJson() const -> QString {
    if(!scene_manager_) {
        return QStringLiteral("{}");
    }
    return QString::fromStdString(scene_manager_->camera_state().to_json().dump());
}

void CoinQuickItem::mousePressEvent(QMouseEvent* event) {
    last_mouse_pos_ = event->position();
    if(event->button() == Qt::LeftButton) {
        is_orbiting_ = true;
        has_last_point_ = false;
    } else if(event->button() == Qt::MiddleButton) {
        is_panning_ = true;
    }
    event->accept();
}

void CoinQuickItem::mouseMoveEvent(QMouseEvent* event) {
    if(!scene_manager_) {
        return;
    }

    const auto pos = event->position();
    const auto width_value = width();
    const auto height_value = height();
    if(width_value <= 0.0 || height_value <= 0.0) {
        last_mouse_pos_ = pos;
        event->accept();
        return;
    }

    if(is_orbiting_ && projector_) {
        auto* cam = scene_manager_->camera();
        if(cam) {
            // Set working space from camera orientation (FreeCAD pattern)
            SbMatrix mat;
            cam->orientation.getValue().getValue(mat);
            projector_->setWorkingSpace(mat);

            SbVec2f norm_pos(
                static_cast<float>(pos.x() / width_value),
                static_cast<float>(1.0 - pos.y() / height_value));

            if(has_last_point_) {
                // Project both prev and curr with the same working space
                projector_->project(last_norm_pos_);
                SbRotation rotation;
                projector_->projectAndGetRotation(norm_pos, rotation);
                rotation.invert();
                // TODO(future): replace hardcoded origin with scene bounding center
                reorient_camera(cam, rotation, SbVec3f(0.F, 0.F, 0.F));
            }

            last_norm_pos_ = norm_pos;
            has_last_point_ = true;
            update();
        }
    } else if(is_panning_) {
        const auto dx = static_cast<float>((pos.x() - last_mouse_pos_.x()) / width_value);
        const auto dy = static_cast<float>((pos.y() - last_mouse_pos_.y()) / height_value);

        auto cam_state = scene_manager_->camera_state();
        auto result = Render::NavigationController::compute_pan(
            cam_state.position, cam_state.orientation, dx, dy, cam_state.focal_distance);

        cam_state.position = result.new_position;
        scene_manager_->restore_camera_state(cam_state);
        update();
    }

    last_mouse_pos_ = pos;
    event->accept();
}

void CoinQuickItem::mouseReleaseEvent(QMouseEvent* event) {
    is_orbiting_ = false;
    is_panning_ = false;
    has_last_point_ = false;
    emit navigationFinished(cameraStateJson());
    event->accept();
}

void CoinQuickItem::wheelEvent(QWheelEvent* event) {
    if(!scene_manager_) {
        return;
    }

    const float delta = static_cast<float>(event->angleDelta().y()) / 120.F;
    auto cam_state = scene_manager_->camera_state();
    const bool is_ortho =
        cam_state.projection == Render::CameraState::ProjectionType::kOrthographic;

    auto result = Render::NavigationController::compute_zoom(
        cam_state.position, cam_state.orientation, cam_state.focal_distance, delta, is_ortho,
        cam_state.height);

    cam_state.position = result.new_position;
    cam_state.focal_distance = result.new_focal_distance;
    if(is_ortho) {
        cam_state.height = result.new_height;
    }
    scene_manager_->restore_camera_state(cam_state);
    update();

    emit navigationFinished(cameraStateJson());
    event->accept();
}

auto CoinQuickItem::displayMode() const -> int {
    if(!scene_manager_) {
        return 0;
    }
    return static_cast<int>(scene_manager_->display_mode());
}

void CoinQuickItem::setDisplayMode(int mode) {
    if(!scene_manager_) {
        return;
    }
    auto dm = static_cast<Render::DisplayMode>(mode);
    if(scene_manager_->display_mode() == dm) {
        return;
    }
    scene_manager_->set_display_mode(dm);
    emit displayModeChanged();
    update();
}

} // namespace OpenGeoLab::App
