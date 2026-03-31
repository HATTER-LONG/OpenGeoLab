/**
 * @file delete_shape_action.hpp
 * @brief DeleteShapeAction — removes a shape from ShapeStore
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Removes a shape from ShapeStore by ID.
 */
class OPENGEOLAB_GEOMETRY_EXPORT DeleteShapeAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit DeleteShapeAction(ShapeStore& store);
    ~DeleteShapeAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"delete_shape"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
