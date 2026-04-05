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
 * @brief Capture viewport metadata (camera, visible shapes, selections, labels).
 *
 * Returns structured JSON describing the current scene state.
 * When image capture is wired (Part 2), also returns a base64-encoded screenshot.
 */
class OPENGEOLAB_SCENE_EXPORT CaptureViewportAction final : public Core::IAction {
public:
    explicit CaptureViewportAction(const SceneGraph& graph);
    ~CaptureViewportAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"capture_viewport"};

private:
    const SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
