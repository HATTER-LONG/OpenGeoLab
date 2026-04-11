/**
 * @file query_entity_info_action.hpp
 * @brief QueryEntityInfoAction — detailed info for a single face/edge/vertex
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Returns detailed information about a single face, edge, or vertex.
 *
 * Includes type-specific properties, bounding box, and adjacency lists.
 * Designed to give an LLM detailed context about a specific entity.
 */
class OPENGEOLAB_GEOMETRY_EXPORT QueryEntityInfoAction final : public Core::IAction {
public:
    explicit QueryEntityInfoAction(ShapeStore& store);
    ~QueryEntityInfoAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"query_entity_info"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
