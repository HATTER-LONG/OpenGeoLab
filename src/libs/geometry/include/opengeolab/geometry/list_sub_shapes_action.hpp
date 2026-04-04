/**
 * @file list_sub_shapes_action.hpp
 * @brief ListSubShapesAction — enumerates sub-shape localIds grouped by type
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Returns all sub-shape localIds for a given shapeId, grouped by entity type.
 *
 * Unlike query_shape (which returns only counts), this action returns the actual
 * 1-based localId arrays so callers can reference individual faces, edges, etc.
 */
class OPENGEOLAB_GEOMETRY_EXPORT ListSubShapesAction final : public Core::IAction {
public:
    explicit ListSubShapesAction(ShapeStore& store);
    ~ListSubShapesAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"list_sub_shapes"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
