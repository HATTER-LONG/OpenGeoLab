/**
 * @file create_cylinder_action.hpp
 * @brief CreateCylinderAction — creates an OCC cylinder primitive
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Creates an OCC cylinder primitive and registers it in ShapeStore.
 */
class OPENGEOLAB_GEOMETRY_EXPORT CreateCylinderAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit CreateCylinderAction(ShapeStore& store);
    ~CreateCylinderAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"create_cylinder"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
