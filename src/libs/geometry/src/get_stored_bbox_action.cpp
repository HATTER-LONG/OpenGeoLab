#include <opengeolab/geometry/get_stored_bbox_action.hpp>

#include <opengeolab/geometry/bounding_box_calculator.hpp>
#include <opengeolab/geometry/point_store.hpp>

#include <utility>

namespace OpenGeoLab::Geometry {

namespace {

auto boundingBoxJson(const BoundingBox& bounding_box) -> nlohmann::json {
    return {
        {"min", {{"x", bounding_box.min.x}, {"y", bounding_box.min.y}, {"z", bounding_box.min.z}}},
        {"max", {{"x", bounding_box.max.x}, {"y", bounding_box.max.y}, {"z", bounding_box.max.z}}},
    };
}

} // namespace

GetStoredBBoxAction::GetStoredBBoxAction(std::shared_ptr<PointStore> store)
    : store_(std::move(store)) {}

auto GetStoredBBoxAction::execute(const nlohmann::json& /*payload*/) -> Base::CommandResult {
    if(store_->empty()) {
        return {
            .ok = false,
            .summary = "No points stored yet. Use geometry.set_points first.",
            .result = {},
        };
    }

    const auto points = store_->points();
    const auto bounding_box = BoundingBoxCalculator::compute(points);

    return Base::CommandResult{
        .ok = true,
        .summary = "Stored bounding box computed.",
        .result =
            {
                {"pointCount", points.size()},
                {"boundingBox", boundingBoxJson(bounding_box)},
            },
    };
}

auto GetStoredBBoxAction::actionName() const noexcept -> std::string_view {
    return "get_stored_bbox";
}

} // namespace OpenGeoLab::Geometry
