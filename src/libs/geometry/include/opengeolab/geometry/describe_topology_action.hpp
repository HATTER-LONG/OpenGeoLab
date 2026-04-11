/**
 * @file describe_topology_action.hpp
 * @brief DescribeTopologyAction — shape topology overview for LLM context
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Returns a structured overview of a shape's topology.
 *
 * Extracts face/edge/vertex counts and per-entity summaries with type,
 * coordinates, dimensions, and bounding box.  Designed to give an LLM
 * enough context to reason about a 3D model.
 *
 * Depends on topology_utils for OCC extraction.
 */
class OPENGEOLAB_GEOMETRY_EXPORT DescribeTopologyAction final : public Core::IAction {
public:
    explicit DescribeTopologyAction(ShapeStore& store);
    ~DescribeTopologyAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"describe_topology"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
