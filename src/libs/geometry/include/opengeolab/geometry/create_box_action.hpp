/**
 * @file create_box_action.hpp
 * @brief CreateBoxAction — simulates a time-consuming box creation
 *
 * This action is a development test harness: it simulates a multi-step
 * computation with progressive LOG_* calls and ProgressCallback reporting,
 * exercising the Activity panel's log view and progress bar.
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

/**
 * @brief Action that simulates creating a box with step-by-step progress.
 *
 * Expected param:
 * @code
 * {
 *   "width":  1.0,   // optional, default 1.0
 *   "height": 1.0,   // optional, default 1.0
 *   "depth":  1.0    // optional, default 1.0
 * }
 * @endcode
 *
 * Returns on success:
 * @code
 * { "ok": true, "action": "create_box", "data": { "width", "height", "depth" } }
 * @endcode
 */
class OPENGEOLAB_GEOMETRY_EXPORT CreateBoxAction final : public Core::IAction {
public:
    CreateBoxAction();
    ~CreateBoxAction() override;

    [[nodiscard]] nlohmann::json describe() const override;

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"create_box"};
};

} // namespace OpenGeoLab::Geometry
