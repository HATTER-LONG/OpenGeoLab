/**
 * @file create_box_action.hpp
 * @brief CreateBoxAction — creates an OCC box primitive and registers it in ShapeStore
 *
 * Expected param:
 * @code
 * {
 *   "width":  1.0,   // optional, default 1.0
 *   "height": 1.0,   // optional, default 1.0
 *   "depth":  1.0,   // optional, default 1.0
 *   "origin": [0, 0, 0],       // optional, default [0,0,0]
 *   "name":   "Box",           // optional, default "Box"
 *   "tessellate": true,        // optional, default true
 *   "linearDeflection":  0.1,  // optional, default 0.1
 *   "angularDeflection": 0.5   // optional, default 0.5
 * }
 * @endcode
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Creates an OCC box primitive and registers it in ShapeStore.
 */
class OPENGEOLAB_GEOMETRY_EXPORT CreateBoxAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit CreateBoxAction(ShapeStore& store);
    ~CreateBoxAction() override;

    [[nodiscard]] nlohmann::json describe() const override;

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"create_box"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
