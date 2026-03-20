#include <opengeolab/geometry/set_points_action.hpp>

#include <opengeolab/geometry/bounding_box_calculator.hpp>
#include <opengeolab/geometry/point_store.hpp>

#include <utility>
#include <vector>

namespace OpenGeoLab::Geometry {

namespace {

auto boundingBoxJson(const BoundingBox& bounding_box) -> nlohmann::json {
    return {
        {"min", {{"x", bounding_box.min.x}, {"y", bounding_box.min.y}, {"z", bounding_box.min.z}}},
        {"max", {{"x", bounding_box.max.x}, {"y", bounding_box.max.y}, {"z", bounding_box.max.z}}},
    };
}

} // namespace

SetPointsAction::SetPointsAction(std::shared_ptr<PointStore> store) : store_(std::move(store)) {}

auto SetPointsAction::execute(const nlohmann::json& payload) -> Command::CommandResult {
    if(!payload.contains("points") || !payload["points"].is_array()) {
        return {
            .ok = false,
            .summary = "Missing or invalid 'points' array.",
            .result = {},
        };
    }

    const auto& json_points = payload["points"];
    if(json_points.empty()) {
        return {
            .ok = false,
            .summary = "Points array must not be empty.",
            .result = {},
        };
    }

    std::vector<Point3D> points;
    points.reserve(json_points.size());
    for(const auto& item : json_points) {
        points.push_back({
            .x = item.value("x", 0.0),
            .y = item.value("y", 0.0),
            .z = item.value("z", 0.0),
        });
    }

    store_->setPoints(points);

    const auto bounding_box = BoundingBoxCalculator::compute(points);
    return Command::CommandResult{
        .ok = true,
        .summary = "Points stored successfully.",
        .result =
            {
                {"stored", true},
                {"pointCount", points.size()},
                {"boundingBox", boundingBoxJson(bounding_box)},
            },
    };
}

auto SetPointsAction::actionName() const noexcept -> std::string_view { return "set_points"; }

} // namespace OpenGeoLab::Geometry
