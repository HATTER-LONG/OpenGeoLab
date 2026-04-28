/**
 * @file rename_shape_action.hpp
 * @brief RenameShapeAction — renames a shape in ShapeStore
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Renames a shape in ShapeStore by ID.
 */
class OPENGEOLAB_GEOMETRY_EXPORT RenameShapeAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit RenameShapeAction(ShapeStore& store);
    ~RenameShapeAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"rename_shape"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
