/**
 * @file capture_viewport_action.hpp
 * @brief CaptureViewportAction — capture viewport metadata for AI context
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Capture viewport metadata and optionally a screenshot.
 *
 * Returns structured JSON with camera, visible shapes, selections,
 * labels, and hover state. When captureImage is true (default),
 * requests an FBO readback from the render thread and returns a
 * base64-encoded PNG screenshot.
 */
class OPENGEOLAB_SCENE_EXPORT CaptureViewportAction final : public Core::IAction {
public:
    explicit CaptureViewportAction(SceneGraph& graph);
    ~CaptureViewportAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"capture_viewport"};

private:
    SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
