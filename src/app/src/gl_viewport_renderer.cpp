/**
 * @file gl_viewport_renderer.cpp
 * @brief GLViewportRenderer implementation
 */

#include <opengeolab/app/gl_viewport_renderer.hpp>

#include <opengeolab/app/gl_viewport.hpp>
#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/render/label_anchor.hpp>
#include <opengeolab/scene/label_manager.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/selection_state.hpp>

#include <glad/gl.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>

#include <algorithm>
#include <cstring>
#include <vector>

namespace OpenGeoLab::App {

namespace {

[[nodiscard]] Render::PickMask pickMaskFromMode(Render::PickMode mode) noexcept {
    switch(mode) {
    case Render::PickMode::VEF:
        return Render::PickMask::Vertex | Render::PickMask::Edge | Render::PickMask::Face;
    case Render::PickMode::Wire:
        return Render::PickMask::Wire;
    case Render::PickMode::Solid:
        return Render::PickMask::Solid;
    case Render::PickMode::Part:
        return Render::PickMask::Part;
    }

    return Render::PickMask::Vertex | Render::PickMask::Edge | Render::PickMask::Face;
}

void fulfillCapturePromise(const Scene::PendingCapture& capture, Scene::CaptureResult result) {
    if(capture.promise) {
        capture.promise->set_value(std::move(result));
    }
}

} // namespace

GLViewportRenderer::GLViewportRenderer() = default;

GLViewportRenderer::~GLViewportRenderer() {
    if(m_pipelineInitialized && QOpenGLContext::currentContext() != nullptr) {
        m_pipeline.cleanup();
    }
}

QOpenGLFramebufferObject* GLViewportRenderer::createFramebufferObject(const QSize& size) {
    static_cast<void>(ensureGladInitialized());

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setInternalTextureFormat(GL_RGBA8);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

void GLViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = static_cast<GLViewport*>(item);
    m_viewport = viewport;

    if(!ensureGladInitialized()) {
        return;
    }

    if(!m_pipelineInitialized) {
        auto gl_loader = +[](const char* name) -> void* {
            if(const auto* const ctx = QOpenGLContext::currentContext(); ctx != nullptr) {
                return reinterpret_cast<void*>(ctx->getProcAddress(name));
            }
            return nullptr;
        };
        m_pipeline.setFontAtlasDir(QCoreApplication::applicationDirPath().toStdString() +
                                   "/resources/fonts");
        m_pipeline.initialize(gl_loader);
        m_pipelineInitialized = true;
    }

    if(const auto* scene = viewport->sceneGraph(); scene != nullptr) {
        [[maybe_unused]] auto lock = scene->readLock();
        m_pipeline.synchronize(*scene);
    }

    const auto* const window = viewport->window();
    const float device_pixel_ratio =
        window != nullptr ? static_cast<float>(window->devicePixelRatio()) : 1.0F;
    const float width_px =
        static_cast<float>(std::max(viewport->width(), 1.0)) * device_pixel_ratio;
    const float height_px =
        static_cast<float>(std::max(viewport->height(), 1.0)) * device_pixel_ratio;
    const float aspect = width_px / std::max(height_px, 1.0F);

    const auto camera = (viewport->sceneGraph() != nullptr)
                            ? viewport->sceneGraph()->viewportState().camera()
                            : Scene::CameraState{};
    m_frameState.viewMatrix = camera.viewMatrix();
    m_frameState.projMatrix = camera.projMatrix(aspect);
    m_frameState.cameraPos = camera.eyePosition();
    m_frameState.viewportWidth = static_cast<int>(width_px);
    m_frameState.viewportHeight = static_cast<int>(height_px);
    m_frameState.devicePixelRatio = device_pixel_ratio;
    // Read display mode from ViewportState (authoritative) with GLViewport fallback.
    if(viewport->sceneGraph() != nullptr) {
        m_frameState.xRayMode = viewport->sceneGraph()->viewportState().xRayMode();
        m_frameState.showTessellation = viewport->sceneGraph()->viewportState().showTessellation();
    } else {
        m_frameState.xRayMode = viewport->xRayMode();
        m_frameState.showTessellation = viewport->showTessellation();
    }

    m_pickingEnabled = viewport->pickingEnabled();
    m_pickMode = static_cast<Render::PickMode>(viewport->pickMode());
    m_pendingPick = viewport->consumePendingPick();
    m_pendingBoxSelect = viewport->consumePendingBoxSelect();
    m_hoverPick = {m_pickingEnabled && viewport->m_hoverActive,
                   static_cast<float>(viewport->m_hoverPos.x()),
                   static_cast<float>(viewport->m_hoverPos.y())};

    // Consume pending pick area from ViewportState (command/Python-driven).
    m_pendingPickArea.reset();
    if(viewport->sceneGraph() != nullptr) {
        m_pendingPickArea = viewport->sceneGraph()->viewportState().consumePickArea();
    }

    // Consume pending viewport capture request.
    m_pendingCapture.reset();
    if(viewport->sceneGraph() != nullptr) {
        m_pendingCapture = viewport->sceneGraph()->viewportState().consumeCapture();
    }

    // Resolve SelectionState → DrawRanges (only when version changes)
    if(const auto* scene = viewport->sceneGraph(); scene != nullptr) {
        const auto& sel = scene->selectionState();
        const uint64_t scene_ver = scene->version();
        const uint64_t sel_ver = sel.selectionVersion();
        const uint64_t hov_ver = sel.hoverVersion();

        // When the scene version changes (visibility toggle, node add/remove),
        // GPU buffers are rebuilt and all DrawRange offsets shift.  Force
        // re-resolution of selection, hover and label caches.
        const bool buffer_dirty = (scene_ver != m_cachedSceneVersion);
        if(buffer_dirty) {
            m_cachedSceneVersion = scene_ver;
        }

        if(sel_ver != m_cachedSelectionVersion || buffer_dirty) {
            m_resolvedSelectedEntries.clear();
            for(const auto& entity : sel.selections()) {
                // Solid / Wire: expand to all shape DrawRanges (face, edge, vertex).
                if(entity.entityType == Core::EntityType::GeoSolid ||
                   entity.entityType == Core::EntityType::GeoWire) {
                    auto shape_ranges = m_pipeline.resolveShapeDrawRanges(entity.shapeId);
                    for(const auto& r : shape_ranges) {
                        m_resolvedSelectedEntries.push_back({r, r.entityType});
                    }
                } else {
                    auto ranges = m_pipeline.resolveEntityDrawRanges(
                        entity.shapeId, entity.entityType, entity.localId);
                    for(const auto& r : ranges) {
                        m_resolvedSelectedEntries.push_back({r, entity.entityType});
                    }
                }
            }
            m_cachedSelectionVersion = sel_ver;
        }

        if(hov_ver != m_cachedHoverVersion || buffer_dirty) {
            m_resolvedHoveredEntries.clear();
            if(const auto hovered = sel.hovered(); hovered.has_value()) {
                if(hovered->entityType == Core::EntityType::GeoSolid ||
                   hovered->entityType == Core::EntityType::GeoWire) {
                    auto shape_ranges = m_pipeline.resolveShapeDrawRanges(hovered->shapeId);
                    for(const auto& r : shape_ranges) {
                        m_resolvedHoveredEntries.push_back({r, r.entityType});
                    }
                } else {
                    auto ranges = m_pipeline.resolveEntityDrawRanges(
                        hovered->shapeId, hovered->entityType, hovered->localId);
                    for(const auto& r : ranges) {
                        m_resolvedHoveredEntries.push_back({r, hovered->entityType});
                    }
                }
            }
            m_cachedHoverVersion = hov_ver;
        }

        // Sync selection-active flag and pick mask for mouse mode mapping
        viewport->setSelectionActive(sel.pickEnabled());
        m_selectionActive = sel.pickEnabled();
        m_selectionPickMask = sel.pickMask();

        // Phase 2: Resolve labels from LabelManager → FrameState
        const auto& lbl_mgr = scene->labelManager();
        m_frameState.labelsVisible = lbl_mgr.isVisible();
        const uint64_t lbl_ver = lbl_mgr.version();

        if(lbl_ver != m_cachedLabelVersion || buffer_dirty) {
            m_frameState.resolvedLabels.clear();
            auto labels = lbl_mgr.labels();

            const auto mvp = m_frameState.projMatrix * m_frameState.viewMatrix;
            const auto vp_w = static_cast<float>(m_frameState.viewportWidth);
            const auto vp_h = static_cast<float>(m_frameState.viewportHeight);

            std::vector<glm::vec2> screen_positions;
            std::vector<Render::ResolvedLabel> resolved;
            resolved.reserve(labels.size());
            screen_positions.reserve(labels.size());

            for(const auto& lbl : labels) {
                glm::vec3 anchor = m_pipeline.resolveEntityAnchor(
                    lbl.entity.shapeId, lbl.entity.entityType, lbl.entity.localId);

                auto clip = mvp * glm::vec4(anchor, 1.0F);
                if(clip.w <= 0.0F) {
                    continue;
                }
                auto ndc = glm::vec3(clip) / clip.w;

                float sx = (ndc.x * 0.5F + 0.5F) * vp_w;
                float sy = (ndc.y * 0.5F + 0.5F) * vp_h;
                screen_positions.emplace_back(sx, sy);

                Render::ResolvedLabel rl;
                rl.anchorWorld = anchor;
                rl.text = lbl.text;
                rl.textColor = lbl.textColor;
                rl.bgColor = lbl.bgColor;
                rl.entityType = lbl.entity.entityType;
                rl.occluded = false;
                rl.stackIndex = 0;
                resolved.push_back(std::move(rl));
            }

            auto stack_indices = Render::computeStackIndices(screen_positions, 4.0F);
            for(std::size_t i = 0; i < resolved.size(); ++i) {
                resolved[i].stackIndex = stack_indices[i];
            }

            m_frameState.resolvedLabels = std::move(resolved);
            m_cachedLabelVersion = lbl_ver;
        }
    }

    m_frameState.selectedEntries = m_resolvedSelectedEntries;
    m_frameState.hoveredEntries = m_resolvedHoveredEntries;
    m_frameState.activePickMask = m_selectionActive ? m_selectionPickMask : Core::PickMask::All;
}

void GLViewportRenderer::render() {
    if(!m_gladInitialized || !m_pipelineInitialized) {
        return;
    }

    m_pipeline.render(m_frameState);

    if(m_pickingEnabled) {
        if(m_hoverPick.active) {
            dispatchHoverResult(pickAtItemPosition(m_hoverPick.x, m_hoverPick.y));
        }

        if(m_pendingPick.active) {
            dispatchPickResult(pickAtItemPosition(m_pendingPick.x, m_pendingPick.y),
                               m_pendingPick.action);
            m_pendingPick = {};
        }

        if(m_pendingBoxSelect.active) {
            dispatchBoxSelectResults(m_pendingBoxSelect);
            m_pendingBoxSelect = {};
        }

        if(m_pendingPickArea.has_value()) {
            dispatchPickAreaResults(*m_pendingPickArea);
            m_pendingPickArea.reset();
        }
    }

    // Viewport capture (independent of picking state).
    if(m_pendingCapture.has_value()) {
        executeCaptureRequest(*m_pendingCapture);
        m_pendingCapture.reset();
    }

    QQuickOpenGLUtils::resetOpenGLState();
}

bool GLViewportRenderer::ensureGladInitialized() {
    if(m_gladInitialized) {
        return true;
    }

    if(QOpenGLContext::currentContext() == nullptr) {
        return false;
    }

    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(+[](const char* name) -> void* {
        if(const auto* const context = QOpenGLContext::currentContext(); context != nullptr) {
            return reinterpret_cast<void*>(context->getProcAddress(name));
        }
        return nullptr;
    }));
    m_gladInitialized = version != 0;
    return m_gladInitialized;
}

Render::PickMask GLViewportRenderer::pickMask() const {
    // When selection mode is active, use SelectionState's pickMask
    // so the GPU pick pass only matches the entity types the user selected.
    if(m_selectionActive && m_selectionPickMask != Render::PickMask::None) {
        return m_selectionPickMask;
    }
    return pickMaskFromMode(m_pickMode);
}

Render::PickResult GLViewportRenderer::pickAtItemPosition(float x, float y) const {
    if(m_frameState.viewportWidth <= 0 || m_frameState.viewportHeight <= 0) {
        return {};
    }

    const float device_x = x * m_frameState.devicePixelRatio;
    const float device_y = y * m_frameState.devicePixelRatio;
    const int pixel_x =
        std::clamp(static_cast<int>(device_x), 0, std::max(m_frameState.viewportWidth - 1, 0));
    const int pixel_y =
        std::clamp(static_cast<int>(device_y), 0, std::max(m_frameState.viewportHeight - 1, 0));
    return m_pipeline.pickAt(pixel_x, pixel_y, pickMask());
}

void GLViewportRenderer::dispatchPickResult(const Render::PickResult& result,
                                            Core::PickAction action) const {
    if(m_viewport.isNull()) {
        return;
    }

    const QPointer<GLViewport> viewport = m_viewport;
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, result, action]() {
            if(viewport.isNull()) {
                return;
            }
            // Preserve existing signal behavior (always emitted)
            viewport->notifyPickResult(result);
            // Selection behavior (only when selection mode is active)
            if(viewport->sceneGraph() == nullptr) {
                return;
            }
            auto& sel = viewport->sceneGraph()->selectionState();
            if(!sel.pickEnabled() || !result.valid) {
                return;
            }
            // Filter: only act on entity types included in the pick mask
            if((Core::maskForEntityType(result.entityType) & sel.pickMask()) ==
               Core::PickMask::None) {
                return;
            }
            const Core::EntityRef entity{result.shapeId, result.entityType, result.localId};
            if(action == Core::PickAction::Add) {
                sel.addSelection(entity);
            } else {
                sel.removeSelection(entity);
            }
        },
        Qt::QueuedConnection);
}

void GLViewportRenderer::dispatchHoverResult(const Render::PickResult& result) const {
    if(m_viewport.isNull()) {
        return;
    }

    const QPointer<GLViewport> viewport = m_viewport;

    if(!result.valid) {
        QMetaObject::invokeMethod(
            viewport.data(),
            [viewport]() {
                if(viewport.isNull()) {
                    return;
                }
                viewport->notifyHoverResult(Render::PickResult{});
                if(viewport->sceneGraph() != nullptr &&
                   viewport->sceneGraph()->selectionState().pickEnabled()) {
                    viewport->sceneGraph()->selectionState().clearHover();
                }
            },
            Qt::QueuedConnection);
        return;
    }

    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, result]() {
            if(viewport.isNull()) {
                return;
            }
            viewport->notifyHoverResult(result);
            if(viewport->sceneGraph() != nullptr &&
               viewport->sceneGraph()->selectionState().pickEnabled()) {
                auto& sel = viewport->sceneGraph()->selectionState();
                // Filter: only hover entities matching the pick mask
                if((Core::maskForEntityType(result.entityType) & sel.pickMask()) ==
                   Core::PickMask::None) {
                    sel.clearHover();
                    return;
                }
                const Core::EntityRef entity{result.shapeId, result.entityType, result.localId};
                sel.setHovered(entity);
            }
        },
        Qt::QueuedConnection);
}

void GLViewportRenderer::dispatchBoxSelectResults(const GLViewport::PendingBoxSelect& box) const {
    if(m_viewport.isNull()) {
        return;
    }

    const float dpr = m_frameState.devicePixelRatio;
    const int px0 = static_cast<int>(box.x1 * dpr);
    const int py0 = static_cast<int>(box.y1 * dpr);
    const int px1 = static_cast<int>(box.x2 * dpr);
    const int py1 = static_cast<int>(box.y2 * dpr);

    auto results = m_pipeline.pickRect(px0, py0, px1, py1, pickMask());
    if(results.empty()) {
        return;
    }

    std::vector<Core::EntityRef> entities;
    entities.reserve(results.size());
    for(const auto& r : results) {
        if(r.valid) {
            // Filter: only accept entity types included in the pick mask.
            if((Core::maskForEntityType(r.entityType) & pickMask()) == Core::PickMask::None) {
                continue;
            }
            entities.push_back({r.shapeId, r.entityType, r.localId});
        }
    }

    const QPointer<GLViewport> viewport = m_viewport;
    const Core::PickAction action = box.action;
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, entities = std::move(entities), action]() {
            if(viewport.isNull() || viewport->sceneGraph() == nullptr) {
                return;
            }
            auto& sel = viewport->sceneGraph()->selectionState();
            for(const auto& entity : entities) {
                if(action == Core::PickAction::Add) {
                    sel.addSelection(entity);
                } else {
                    sel.removeSelection(entity);
                }
            }
        },
        Qt::QueuedConnection);
}

void GLViewportRenderer::dispatchPickAreaResults(const Scene::PendingPickArea& area) const {
    if(m_viewport.isNull()) {
        return;
    }

    float fx0 = area.x0;
    float fy0 = area.y0;
    float fx1 = area.x1;
    float fy1 = area.y1;

    if(area.coordType == Scene::PickAreaCoordType::Normalized) {
        fx0 *= static_cast<float>(m_frameState.viewportWidth);
        fy0 *= static_cast<float>(m_frameState.viewportHeight);
        fx1 *= static_cast<float>(m_frameState.viewportWidth);
        fy1 *= static_cast<float>(m_frameState.viewportHeight);
    } else {
        const float dpr = m_frameState.devicePixelRatio;
        fx0 *= dpr;
        fy0 *= dpr;
        fx1 *= dpr;
        fy1 *= dpr;
    }

    auto results = m_pipeline.pickRect(static_cast<int>(fx0), static_cast<int>(fy0),
                                       static_cast<int>(fx1), static_cast<int>(fy1), pickMask());
    if(results.empty()) {
        return;
    }

    std::vector<Core::EntityRef> entities;
    entities.reserve(results.size());
    for(const auto& r : results) {
        if(r.valid) {
            if((Core::maskForEntityType(r.entityType) & pickMask()) == Core::PickMask::None) {
                continue;
            }
            entities.push_back({r.shapeId, r.entityType, r.localId});
        }
    }

    const QPointer<GLViewport> viewport = m_viewport;
    const Core::PickAction action = area.action;
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, entities = std::move(entities), action]() {
            if(viewport.isNull() || viewport->sceneGraph() == nullptr) {
                return;
            }
            auto& sel = viewport->sceneGraph()->selectionState();
            for(const auto& entity : entities) {
                if(action == Core::PickAction::Add) {
                    sel.addSelection(entity);
                } else {
                    sel.removeSelection(entity);
                }
            }
        },
        Qt::QueuedConnection);
}

void GLViewportRenderer::executeCaptureRequest(const Scene::PendingCapture& capture) {
    const int w = m_frameState.viewportWidth;
    const int h = m_frameState.viewportHeight;

    if(w <= 0 || h <= 0) {
        Scene::CaptureResult result;
        result.savedPathError = "Capture failed because the viewport size is invalid.";
        fulfillCapturePromise(capture, std::move(result));
        return;
    }

    // The viewport FBO uses 4× MSAA, so direct glReadPixels yields
    // undefined data.  QOpenGLFramebufferObject::toImage() resolves the
    // multisample buffer, reads the pixels and flips the image in one step.
    // Convert to RGB32 (opaque) so the saved PNG matches the on-screen
    // composite regardless of FBO alpha state.
    QImage image = framebufferObject()->toImage().convertToFormat(QImage::Format_RGB32);

    Scene::CaptureResult result;

    const QString output_path = QString::fromStdString(capture.outputPath);
    const QFileInfo output_file(output_path);
    QDir output_dir = output_file.dir();
    if(!output_dir.exists() && !output_dir.mkpath(QStringLiteral("."))) {
        result.savedPathError = "Failed to create the output directory for filePath.";
    } else if(!image.save(output_path, "PNG")) {
        result.savedPathError = "Failed to write PNG to filePath.";
    } else {
        result.savedPath = capture.outputPath;
    }

    fulfillCapturePromise(capture, std::move(result));
}

} // namespace OpenGeoLab::App
