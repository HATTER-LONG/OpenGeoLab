/**
 * @file create_torus_action.hpp
 * @brief CreateTorusAction — creates an OCC torus primitive
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Creates an OCC torus primitive and registers it in ShapeStore.
 */
class OPENGEOLAB_GEOMETRY_EXPORT CreateTorusAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit CreateTorusAction(ShapeStore& store);
    ~CreateTorusAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"create_torus"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
