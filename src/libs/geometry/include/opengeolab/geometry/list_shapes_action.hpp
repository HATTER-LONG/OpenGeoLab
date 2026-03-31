/**
 * @file list_shapes_action.hpp
 * @brief ListShapesAction — lists all shapes in ShapeStore
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Enumerates all shapes currently held in ShapeStore.
 */
class OPENGEOLAB_GEOMETRY_EXPORT ListShapesAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit ListShapesAction(ShapeStore& store);
    ~ListShapesAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"list_shapes"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
