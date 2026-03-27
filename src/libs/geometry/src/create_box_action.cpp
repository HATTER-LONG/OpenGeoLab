/**
 * @file create_box_action.cpp
 * @brief CreateBoxAction implementation — simulated time-consuming operation
 *
 * Iterates through 10 steps with 300ms sleeps, emitting LOG_INFO/WARN/DEBUG
 * at each stage and reporting progress via ProgressCallback. Designed to
 * exercise the Activity panel log view and progress indicators.
 */

#include <opengeolab/geometry/create_box_action.hpp>

#include <opengeolab/core/logger.hpp>

#include <fmt/format.h>

#include <chrono>
#include <thread>

namespace OpenGeoLab::Geometry {

CreateBoxAction::CreateBoxAction() = default;
CreateBoxAction::~CreateBoxAction() = default;

nlohmann::json CreateBoxAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Create a box primitive (simulated long-running operation for testing)."},
        {"params",
         {{"width",
           {{"type", "number"}, {"required", false}, {"description", "Box width (default 1.0)"}}},
          {"height",
           {{"type", "number"}, {"required", false}, {"description", "Box height (default 1.0)"}}},
          {"depth",
           {{"type", "number"}, {"required", false}, {"description", "Box depth (default 1.0)"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success"}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name"}}},
          {"data",
           {{"type", "object"},
            {"description", "Created box dimensions (width, height, depth)"}}}}}};
}

nlohmann::json CreateBoxAction::execute(const nlohmann::json& param,
                                        const Core::ProgressCallback& progress) {
    const double width = param.value("width", 1.0);
    const double height = param.value("height", 1.0);
    const double depth = param.value("depth", 1.0);

    LOG_INFO("CreateBoxAction: creating box ({:.2f} x {:.2f} x {:.2f})", width, height, depth);

    if(progress) {
        progress(0.0, "Initializing box creation...");
    }

    constexpr int total_steps = 10;
    constexpr auto step_delay = std::chrono::milliseconds(300);

    for(int step = 1; step <= total_steps; ++step) {
        std::this_thread::sleep_for(step_delay);

        const double pct = static_cast<double>(step) / total_steps;

        if(step <= 3) {
            LOG_INFO("CreateBoxAction: step {}/{} — computing vertices...", step, total_steps);
        } else if(step <= 6) {
            LOG_INFO("CreateBoxAction: step {}/{} — generating faces...", step, total_steps);
        } else if(step <= 8) {
            LOG_DEBUG("CreateBoxAction: step {}/{} — optimizing mesh...", step, total_steps);
        } else if(step == 9) {
            LOG_WARN("CreateBoxAction: step {}/{} — heavy computation phase", step, total_steps);
        } else {
            LOG_INFO("CreateBoxAction: step {}/{} — finalizing...", step, total_steps);
        }

        if(progress && !progress(pct, fmt::format("Step {}/{}", step, total_steps))) {
            LOG_WARN("CreateBoxAction: cancelled at step {}", step);
            return {{"ok", false}, {"summary", "Cancelled by user"}};
        }
    }

    LOG_INFO("CreateBoxAction: box created successfully");
    return {{"ok", true},
            {"action", "create_box"},
            {"data", {{"width", width}, {"height", height}, {"depth", depth}}}};
}

} // namespace OpenGeoLab::Geometry
