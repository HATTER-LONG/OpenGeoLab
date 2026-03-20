#include <opengeolab/render/RenderService.hpp>

#include <stdexcept>
#include <string>
#include <string_view>

namespace OpenGeoLab::Render
{

namespace
{

constexpr std::string_view PLACEHOLDER_PNG_BASE64 {
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+c1ioAAAAASUVORK5CYII="
};

const nlohmann::json& ensureObject(
    const nlohmann::json& payload,
    std::string_view context
)
{
    if (!payload.is_object()) {
        throw std::invalid_argument(std::string(context) + " must be a JSON object");
    }

    return payload;
}

[[nodiscard]] int readPositiveInt(
    const nlohmann::json& payload,
    std::string_view key,
    int default_value
)
{
    const int value = payload.value(std::string(key), default_value);
    if (value <= 0) {
        throw std::invalid_argument(std::string(key) + " must be greater than zero");
    }

    return value;
}

[[nodiscard]] double readPositiveDouble(
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

[[nodiscard]] double readNumber(
    const nlohmann::json& payload,
    std::string_view key,
    double default_value
)
{
    return payload.value(std::string(key), default_value);
}

[[nodiscard]] nlohmann::json readPoint3(
    const nlohmann::json& payload,
    std::string_view key,
    const nlohmann::json& default_value
)
{
    const nlohmann::json point
        = payload.contains(std::string(key)) ? payload.at(std::string(key)) : default_value;
    const auto& point_object = ensureObject(point, std::string(key) + " point");

    return {
        {"x", point_object.value("x", default_value.at("x").get<double>())},
        {"y", point_object.value("y", default_value.at("y").get<double>())},
        {"z", point_object.value("z", default_value.at("z").get<double>())}
    };
}

[[nodiscard]] nlohmann::json normalizeCamera(const nlohmann::json& payload)
{
    static const nlohmann::json DEFAULT_TARGET {
        {"x", 0.0},
        {"y", 0.0},
        {"z", 0.0}
    };

    const auto& camera_payload = ensureObject(payload, "camera");

    return {
        {"target", readPoint3(camera_payload, "target", DEFAULT_TARGET)},
        {"distance", readPositiveDouble(camera_payload, "distance", 8.0)},
        {"azimuthDeg", readNumber(camera_payload, "azimuthDeg", 35.0)},
        {"elevationDeg", readNumber(camera_payload, "elevationDeg", 25.0)},
        {"rollDeg", readNumber(camera_payload, "rollDeg", 0.0)},
        {"up", {{"x", 0.0}, {"y", 0.0}, {"z", 1.0}}}
    };
}

[[nodiscard]] nlohmann::json normalizeViewport(const nlohmann::json& payload)
{
    const auto& object_payload = ensureObject(payload, "render payload");
    const nlohmann::json camera_payload = object_payload.value("camera", nlohmann::json::object());

    return {
        {"viewportId", object_payload.value("viewportId", "mainViewport")},
        {"width", readPositiveInt(object_payload, "width", 1280)},
        {"height", readPositiveInt(object_payload, "height", 720)},
        {"projection", object_payload.value("projection", "perspective")},
        {"cameraModel", object_payload.value("cameraModel", "orbit")},
        {"camera", normalizeCamera(camera_payload)},
        {"replayBoundary",
         {{"kind", "view-state"},
          {"recordRawInput", false},
          {"headlessReady", true},
          {"reason", "Persist explicit camera state instead of raw mouse deltas."}}}
    };
}

}  // namespace

nlohmann::json RenderService::describeViewport(const nlohmann::json& payload)
{
    return normalizeViewport(payload);
}

nlohmann::json RenderService::captureSnapshot(const nlohmann::json& payload)
{
    const auto viewport = normalizeViewport(payload);

    return {
        {"snapshot",
         {{"mimeType", "image/png"},
          {"encoding", "base64"},
          {"width", 1},
          {"height", 1},
          {"data", std::string(PLACEHOLDER_PNG_BASE64)},
          {"summary", "Placeholder snapshot paired with an explicit replayable viewport state."}}},
        {"viewport", viewport},
        {"pipeline",
         {{"implementation", "placeholder"},
          {"nextSteps", {"syncScene", "gpuPicking", "headlessFrustumQuery"}}}}
    };
}

}  // namespace OpenGeoLab::Render
