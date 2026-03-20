#include <opengeolab/geometry/bounding_box_action.hpp>

#include <opengeolab/geometry/bounding_box_calculator.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace OpenGeoLab::Geometry {

namespace {

constexpr std::size_t MAX_POINT_COUNT = 100'000'000;
constexpr std::size_t DEFAULT_POINT_COUNT = 1'000'000;

auto boundingBoxJson(const BoundingBox& bounding_box) -> nlohmann::json {
    return {
        {"min", {{"x", bounding_box.min.x}, {"y", bounding_box.min.y}, {"z", bounding_box.min.z}}},
        {"max", {{"x", bounding_box.max.x}, {"y", bounding_box.max.y}, {"z", bounding_box.max.z}}},
    };
}

} // namespace

auto BoundingBoxAction::execute(const nlohmann::json& payload) -> Command::CommandResult {
    if(payload.contains("pointCount") && payload["pointCount"].is_number_integer() &&
       payload["pointCount"].get<std::int64_t>() < 0) {
        throw std::invalid_argument("pointCount must be non-negative");
    }

    const std::size_t point_count = payload.value("pointCount", DEFAULT_POINT_COUNT);
    if(point_count == 0 || point_count > MAX_POINT_COUNT) {
        throw std::invalid_argument("pointCount must be in range [1, 100000000]");
    }

    const unsigned int seed = payload.value("seed", 42U);
    const auto start_time = std::chrono::steady_clock::now();
    const auto points = BoundingBoxCalculator::generateRandomPoints(point_count, seed);
    const BoundingBox bounding_box = BoundingBoxCalculator::compute(points);
    const auto end_time = std::chrono::steady_clock::now();
    const auto elapsed_ms =
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    const auto bounding_box_json = boundingBoxJson(bounding_box);

    return Command::CommandResult{
        .ok = true,
        .summary = "Computed bounding box.",
        .result =
            {
                {"min", bounding_box_json.at("min")},
                {"max", bounding_box_json.at("max")},
                {"pointCount", point_count},
                {"elapsedMs", elapsed_ms},
            },
    };
}

auto BoundingBoxAction::actionName() const noexcept -> std::string_view { return "bounding_box"; }

} // namespace OpenGeoLab::Geometry
