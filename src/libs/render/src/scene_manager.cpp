/**
 * @file scene_manager.cpp
 * @brief SceneManager implementation using Coin3D scene graph.
 */

#include <opengeolab/render/scene_manager.hpp>

#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

namespace OpenGeoLab::Render {

namespace {

auto build_edge_group(const int32_t* indices, int num_indices) -> SoSeparator* {
    auto* edge_group = new SoSeparator;

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(0.F, 0.F, 0.F);
    edge_group->addChild(material);

    auto* draw_style = new SoDrawStyle;
    draw_style->style = SoDrawStyle::LINES;
    draw_style->lineWidth = 2.F;
    edge_group->addChild(draw_style);

    auto* light_model = new SoLightModel;
    light_model->model = SoLightModel::BASE_COLOR;
    edge_group->addChild(light_model);

    auto* line_set = new SoIndexedLineSet;
    line_set->coordIndex.setValues(0, num_indices, indices);
    edge_group->addChild(line_set);

    return edge_group;
}

auto build_vertex_group() -> SoSeparator* {
    auto* vertex_group = new SoSeparator;

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(0.8F, 0.2F, 0.2F);
    vertex_group->addChild(material);

    auto* draw_style = new SoDrawStyle;
    draw_style->style = SoDrawStyle::POINTS;
    draw_style->pointSize = 5.F;
    vertex_group->addChild(draw_style);

    auto* light_model = new SoLightModel;
    light_model->model = SoLightModel::BASE_COLOR;
    vertex_group->addChild(light_model);

    auto* point_set = new SoPointSet;
    point_set->numPoints = 8;
    vertex_group->addChild(point_set);

    return vertex_group;
}

} // namespace

struct SceneManager::Impl {
    SoSeparator* root = nullptr;
    SoCamera* camera = nullptr;
    SoDirectionalLight* headlight = nullptr;
    SoSeparator* object_group = nullptr;
    DisplayMode current_display_mode = DisplayMode::kFlatLines;
    std::unordered_map<std::string, SoSwitch*> mode_switches;
    std::unordered_map<std::string, SoSeparator*> nodes;
    int next_node_id = 0;
    bool initialized = false;
};

SceneManager::SceneManager() : impl_(std::make_unique<Impl>()) {}

SceneManager::~SceneManager() {
    if(impl_ && impl_->root) {
        impl_->root->unref();
    }
}

SceneManager::SceneManager(SceneManager&&) noexcept = default;
auto SceneManager::operator=(SceneManager&&) noexcept -> SceneManager& = default;

void SceneManager::initialize() {
    if(impl_->initialized) {
        return;
    }
    SoDB::init();

    impl_->root = new SoSeparator;
    impl_->root->ref();

    auto* cam = new SoPerspectiveCamera;
    cam->position.setValue(0.F, 0.F, 10.F);
    cam->nearDistance.setValue(0.1F);
    cam->farDistance.setValue(1000.F);
    cam->focalDistance.setValue(5.F);
    impl_->camera = cam;

    impl_->headlight = new SoDirectionalLight;
    impl_->object_group = new SoSeparator;

    impl_->root->addChild(impl_->camera);
    impl_->root->addChild(impl_->headlight);
    impl_->root->addChild(impl_->object_group);

    impl_->initialized = true;
}

auto SceneManager::root_node() const -> SoSeparator* { return impl_->root; }

auto SceneManager::camera() const -> SoCamera* { return impl_->camera; }

auto SceneManager::add_box(float size_x, float size_y, float size_z) -> std::string {
    auto* node_sep = new SoSeparator;

    auto* coords = new SoCoordinate3;
    const float hx = size_x / 2.F;
    const float hy = size_y / 2.F;
    const float hz = size_z / 2.F;
    coords->point.setNum(8);
    coords->point.set1Value(0, -hx, -hy, -hz);
    coords->point.set1Value(1, hx, -hy, -hz);
    coords->point.set1Value(2, hx, hy, -hz);
    coords->point.set1Value(3, -hx, hy, -hz);
    coords->point.set1Value(4, -hx, -hy, hz);
    coords->point.set1Value(5, hx, -hy, hz);
    coords->point.set1Value(6, hx, hy, hz);
    coords->point.set1Value(7, -hx, hy, hz);
    node_sep->addChild(coords);

    auto* normals = new SoNormal;
    normals->vector.setNum(6);
    normals->vector.set1Value(0, 0.F, 0.F, 1.F);  // front (+Z)
    normals->vector.set1Value(1, 0.F, 0.F, -1.F); // back  (-Z)
    normals->vector.set1Value(2, 0.F, 1.F, 0.F);  // top   (+Y)
    normals->vector.set1Value(3, 0.F, -1.F, 0.F); // bottom(-Y)
    normals->vector.set1Value(4, 1.F, 0.F, 0.F);  // right (+X)
    normals->vector.set1Value(5, -1.F, 0.F, 0.F); // left  (-X)
    node_sep->addChild(normals);

    auto* normal_binding = new SoNormalBinding;
    normal_binding->value = SoNormalBinding::PER_FACE_INDEXED;
    node_sep->addChild(normal_binding);

    auto* face_set = new SoIndexedFaceSet;

    // 12 triangles (6 faces × 2), each terminated by -1
    // clang-format off
    const int32_t coord_indices[] = {
        4, 5, 6, -1,  4, 6, 7, -1,   // front  (+Z)
        1, 0, 3, -1,  1, 3, 2, -1,   // back   (-Z)
        3, 7, 6, -1,  3, 6, 2, -1,   // top    (+Y)
        0, 1, 5, -1,  0, 5, 4, -1,   // bottom (-Y)
        1, 2, 6, -1,  1, 6, 5, -1,   // right  (+X)
        0, 4, 7, -1,  0, 7, 3, -1    // left   (-X)
    };
    const int32_t normal_indices[] = {
        0, -1, 0, -1,
        1, -1, 1, -1,
        2, -1, 2, -1,
        3, -1, 3, -1,
        4, -1, 4, -1,
        5, -1, 5, -1
    };
    // clang-format on

    face_set->coordIndex.setValues(0, 48, coord_indices);
    face_set->normalIndex.setValues(0, 24, normal_indices);

    constexpr int32_t edge_indices[] = {0, 1, -1, 1, 5, -1, 5, 4, -1, 4, 0, -1, 3, 2, -1, 2, 6, -1,
                                        6, 7, -1, 7, 3, -1, 0, 3, -1, 1, 2, -1, 5, 6, -1, 4, 7, -1};

    auto* mode_switch = new SoSwitch;
    mode_switch->whichChild = (impl_->current_display_mode == DisplayMode::kFlatLines) ? 0 : 1;

    auto* flat_lines_root = new SoSeparator;
    auto* face_group = new SoSeparator;

    auto* polygon_offset = new SoPolygonOffset;
    polygon_offset->factor = 1.0F;
    polygon_offset->units = 1.0F;
    polygon_offset->styles = SoPolygonOffset::FILLED;
    polygon_offset->on = TRUE;
    face_group->addChild(polygon_offset);

    auto* face_material = new SoMaterial;
    face_material->diffuseColor.setValue(0.48F, 0.64F, 0.78F);
    face_group->addChild(face_material);

    auto* face_draw_style = new SoDrawStyle;
    face_draw_style->style = SoDrawStyle::FILLED;
    face_group->addChild(face_draw_style);
    face_group->addChild(face_set);

    flat_lines_root->addChild(face_group);
    flat_lines_root->addChild(build_edge_group(edge_indices, 36));
    flat_lines_root->addChild(build_vertex_group());

    auto* wireframe_root = new SoSeparator;
    wireframe_root->addChild(build_edge_group(edge_indices, 36));
    wireframe_root->addChild(build_vertex_group());

    mode_switch->addChild(flat_lines_root);
    mode_switch->addChild(wireframe_root);
    node_sep->addChild(mode_switch);

    impl_->object_group->addChild(node_sep);

    auto id = "box_" + std::to_string(impl_->next_node_id++);
    impl_->nodes[id] = node_sep;
    impl_->mode_switches[id] = mode_switch;
    return id;
}

void SceneManager::remove_node(std::string_view node_id) {
    const auto id = std::string(node_id);
    auto it = impl_->nodes.find(id);
    if(it == impl_->nodes.end()) {
        return;
    }
    impl_->object_group->removeChild(it->second);
    impl_->nodes.erase(it);
    impl_->mode_switches.erase(id);
}

void SceneManager::set_display_mode(DisplayMode mode) {
    impl_->current_display_mode = mode;
    const int child = (mode == DisplayMode::kFlatLines) ? 0 : 1;
    for(auto& [id, sw] : impl_->mode_switches) {
        sw->whichChild = child;
    }
}

auto SceneManager::display_mode() const -> DisplayMode { return impl_->current_display_mode; }

auto SceneManager::camera_state() const -> CameraState {
    CameraState state;
    if(!impl_->camera) {
        return state;
    }

    const auto& pos = impl_->camera->position.getValue();
    state.position = {pos[0], pos[1], pos[2]};

    float qx = 0.F;
    float qy = 0.F;
    float qz = 0.F;
    float qw = 1.F;
    impl_->camera->orientation.getValue().getValue(qx, qy, qz, qw);
    state.orientation = {qx, qy, qz, qw};

    state.near_distance = impl_->camera->nearDistance.getValue();
    state.far_distance = impl_->camera->farDistance.getValue();
    state.focal_distance = impl_->camera->focalDistance.getValue();

    if(auto* persp = dynamic_cast<SoPerspectiveCamera*>(impl_->camera)) {
        state.projection = CameraState::ProjectionType::kPerspective;
        state.height_angle = persp->heightAngle.getValue();
    } else if(auto* ortho = dynamic_cast<SoOrthographicCamera*>(impl_->camera)) {
        state.projection = CameraState::ProjectionType::kOrthographic;
        state.height = ortho->height.getValue();
    }
    return state;
}

void SceneManager::restore_camera_state(const CameraState& state) {
    if(!impl_->camera) {
        return;
    }

    impl_->camera->position.setValue(state.position[0], state.position[1], state.position[2]);
    impl_->camera->orientation.setValue(SbRotation(state.orientation[0], state.orientation[1],
                                                   state.orientation[2], state.orientation[3]));
    impl_->camera->nearDistance.setValue(state.near_distance);
    impl_->camera->farDistance.setValue(state.far_distance);
    impl_->camera->focalDistance.setValue(state.focal_distance);

    if(auto* persp = dynamic_cast<SoPerspectiveCamera*>(impl_->camera)) {
        persp->heightAngle.setValue(state.height_angle);
    } else if(auto* ortho = dynamic_cast<SoOrthographicCamera*>(impl_->camera)) {
        ortho->height.setValue(state.height);
    }
}

void SceneManager::view_all(int viewport_width, int viewport_height) {
    if(!impl_->camera || !impl_->root) {
        return;
    }
    const SbViewportRegion vp(viewport_width, viewport_height);
    impl_->camera->viewAll(impl_->root, vp);
}

auto SceneManager::describe_scene() const -> nlohmann::json {
    auto nodes_array = nlohmann::json::array();
    for(const auto& [id, node] : impl_->nodes) {
        nodes_array.push_back({{"id", id}, {"type", "box"}});
    }
    return {{"nodes", nodes_array}};
}

} // namespace OpenGeoLab::Render
