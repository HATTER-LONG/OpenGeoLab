#include <opengeolab/command/bounding_box_command.hpp>

#include <opengeolab/geometry/bounding_box_calculator.hpp>

#include <chrono>
#include <cstddef>

namespace OpenGeoLab::Command {

auto BoundingBoxCommand::execute(const nlohmann::json& payload) -> CommandResult {
    const std::size_t point_count =
        payload.value("pointCount", static_cast<std::size_t>(1'000'000));
    const unsigned int seed = payload.value("seed", 42U);

    const auto start_time = std::chrono::steady_clock::now();
    const auto points =
        OpenGeoLab::Geometry::BoundingBoxCalculator::generateRandomPoints(point_count, seed);
    const OpenGeoLab::Geometry::BoundingBox bounding_box =
        OpenGeoLab::Geometry::BoundingBoxCalculator::compute(points);
    const auto end_time = std::chrono::steady_clock::now();

    const auto elapsed_ms =
        std::chrono::duration<double, std::milli>(end_time - start_time).count();

    return CommandResult{
        .ok = true,
        .summary = "Computed bounding box.",
        .result =
            {
                {"min",
                 {{"x", bounding_box.min.x}, {"y", bounding_box.min.y}, {"z", bounding_box.min.z}}},
                {"max",
                 {{"x", bounding_box.max.x}, {"y", bounding_box.max.y}, {"z", bounding_box.max.z}}},
                {"pointCount", point_count},
                {"elapsedMs", elapsed_ms},
            },
    };
}

auto BoundingBoxCommand::actionName() const noexcept -> std::string_view {
    return "geometry.bounding_box";
}

} // namespace OpenGeoLab::Command
