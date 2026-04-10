/**
 * @file delete_entity_action.hpp
 * @brief DeleteEntityAction — removes sub-entities (faces, solids) from shapes
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Removes sub-entities from shapes via defeaturing or compound rebuild.
 *
 * Accepts an array of EntityRef-like objects, groups them by shapeId, and:
 * - GeoFace: uses BRepAlgoAPI_Defeaturing to remove faces and heal the solid.
 * - GeoSolid: rebuilds the compound without the target solids.
 *
 * If all sub-entities are removed, the entire shape is deleted from the store.
 * After modification, the shape is re-tessellated automatically.
 */
class OPENGEOLAB_GEOMETRY_EXPORT DeleteEntityAction final : public Core::IAction {
public:
    explicit DeleteEntityAction(ShapeStore& store);
    ~DeleteEntityAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"delete_entity"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
