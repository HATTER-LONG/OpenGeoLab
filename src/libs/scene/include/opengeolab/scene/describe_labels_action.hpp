/**
 * @file describe_labels_action.hpp
 * @brief DescribeLabelsAction — return label state and visual encoding for LLM
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/**
 * @brief Return active labels and visual encoding scheme for LLM observability.
 *
 * Always includes the color legend (entity type → color + prefix) even when
 * there are no active labels, so an LLM can learn the encoding before any
 * selections are made.
 */
class OPENGEOLAB_SCENE_EXPORT DescribeLabelsAction final : public Core::IAction {
public:
    explicit DescribeLabelsAction(const LabelManager& label_manager);
    ~DescribeLabelsAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "describe_labels";

private:
    const LabelManager& m_labelManager;

    [[nodiscard]] static nlohmann::json buildColorLegend();
};

} // namespace OpenGeoLab::Scene
