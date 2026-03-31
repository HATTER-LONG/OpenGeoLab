/**
 * @file create_sphere_action.hpp
 * @brief CreateSphereAction — creates an OCC sphere primitive
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Creates an OCC sphere primitive and registers it in ShapeStore.
 */
class OPENGEOLAB_GEOMETRY_EXPORT CreateSphereAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit CreateSphereAction(ShapeStore& store);
    ~CreateSphereAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"create_sphere"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
