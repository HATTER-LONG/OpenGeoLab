#include <opengeolab/geometry/GeometryService.hpp>

#include <cmath>
#include <stdexcept>

namespace OpenGeoLab::Geometry
{

namespace
{

[[nodiscard]] double readPositiveDimension(
    const nlohmann::json& payload,
    std::string_view key,
    double default_value
)
{
    const double value = payload.value(std::string(key), default_value);
    if (value <= 0.0) {
        throw std::invalid_argument(std::string(key) + " must be greater than zero");
    }

    return value;
}

}  // namespace

nlohmann::json GeometryService::describeBox(const nlohmann::json& payload)
{
    const double width = readPositiveDimension(payload, "width", 1.0);
    const double height = readPositiveDimension(payload, "height", 1.0);
    const double depth = readPositiveDimension(payload, "depth", 1.0);

    const double volume = width * height * depth;
    const double surface_area = 2.0 * ((width * height) + (width * depth) + (height * depth));
    const double diagonal = std::sqrt((width * width) + (height * height) + (depth * depth));

    return {
        {"kind", "box"},
        {"dimensions", {{"width", width}, {"height", height}, {"depth", depth}}},
        {"metrics", {{"volume", volume}, {"surfaceArea", surface_area}, {"diagonal", diagonal}}},
        {"llmHints",
         {{"summary", "Axis-aligned placeholder solid used by the lightweight backend skeleton."},
          {"topology", {{"faces", 6}, {"edges", 12}, {"vertices", 8}}},
          {"selectionEntity", "box://demo/0"}}}
    };
}

}  // namespace OpenGeoLab::Geometry
