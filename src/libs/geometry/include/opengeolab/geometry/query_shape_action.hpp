/**
 * @file query_shape_action.hpp
 * @brief QueryShapeAction — returns topology info and bounding box of a shape
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Returns topology info and bounding box of a shape.
 */
class OPENGEOLAB_GEOMETRY_EXPORT QueryShapeAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit QueryShapeAction(ShapeStore& store);
    ~QueryShapeAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"query_shape"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
