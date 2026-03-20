#include <opengeolab/selection/SelectionService.hpp>

#include <opengeolab/render/RenderService.hpp>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Selection
{

namespace
{

[[nodiscard]] const nlohmann::json& ensureObject(
    const nlohmann::json& payload,
    std::string_view context
)
{
    if (!payload.is_object()) {
        throw std::invalid_argument(std::string(context) + " must be a JSON object");
    }

    return payload;
}

[[nodiscard]] std::vector<std::string> readEntityKinds(const nlohmann::json& payload)
{
    auto entity_kinds = payload.value("entityKinds", std::vector<std::string> {"face"});
    if (entity_kinds.empty()) {
        entity_kinds.emplace_back("face");
    }

    return entity_kinds;
}

[[nodiscard]] nlohmann::json buildPrimaryHit(std::string_view entity_kind)
{
    std::string sub_element {"Face1"};
    if (entity_kind == "edge") {
        sub_element = "Edge1";
    }
    else if (entity_kind == "vertex") {
        sub_element = "Vertex1";
    }

    return {
        {"entityId", "box://demo/0"},
        {"entityKind", std::string(entity_kind)},
        {"subElement", sub_element},
        {"confidence", 0.0}
    };
}

[[nodiscard]] nlohmann::json buildPlaceholderHits(const std::vector<std::string>& entity_kinds)
{
    auto hits = nlohmann::json::array();
    for (const auto& entity_kind : entity_kinds) {
        hits.push_back(buildPrimaryHit(entity_kind));
    }

    return hits;
}

[[nodiscard]] nlohmann::json normalizeRectangle(const nlohmann::json& payload)
{
    const auto& object_payload = ensureObject(payload, "selection rectangle");

    const int left = object_payload.value("left", 120);
    const int top = object_payload.value("top", 100);
    const int right = object_payload.value("right", 600);
    const int bottom = object_payload.value("bottom", 420);

    if (right <= left || bottom <= top) {
        throw std::invalid_argument("selection rectangle must have positive area");
    }

    return {
        {"left", left},
        {"top", top},
        {"right", right},
        {"bottom", bottom}
    };
}

[[nodiscard]] nlohmann::json readViewportPayload(const nlohmann::json& payload)
{
    return payload.contains("viewport") ? payload.at("viewport") : payload;
}

}  // namespace

nlohmann::json SelectionService::describePick(const nlohmann::json& payload)
{
    const auto& object_payload = ensureObject(payload, "selection payload");
    const auto entity_kinds = readEntityKinds(object_payload);

    return {
        {"queryKind", "pick"},
        {"screenPosition",
         {{"x", object_payload.value("screenX", 0)}, {"y", object_payload.value("screenY", 0)}}},
        {"viewport", OpenGeoLab::Render::RenderService::describeViewport(readViewportPayload(payload))},
        {"filters", {{"entityKinds", entity_kinds}}},
        {"hit", buildPrimaryHit(entity_kinds.front())},
        {"selectionIntent",
         {{"operation", object_payload.value("replace", true) ? "replace" : "append"},
          {"entityKinds", entity_kinds}}},
        {"queryModel",
         {{"interactive", "screen-point"}, {"headlessEquivalent", "ray-cast-with-view-state"}}},
        {"replayBoundary",
         {{"kind", "selection-query"},
          {"recordRawInput", false},
          {"headlessReady", true},
          {"reason", "Replay the pick with explicit viewport state instead of mouse device state."}}}
    };
}

nlohmann::json SelectionService::describeBoxSelection(const nlohmann::json& payload)
{
    const auto& object_payload = ensureObject(payload, "selection payload");
    const auto entity_kinds = readEntityKinds(object_payload);
    const bool replace = object_payload.value("replace", true);

    return {
        {"queryKind", "box"},
        {"selectionMode", "box"},
        {"viewport", OpenGeoLab::Render::RenderService::describeViewport(readViewportPayload(payload))},
        {"rectangle", normalizeRectangle(object_payload.value("rectangle", nlohmann::json::object()))},
        {"filters",
         {{"entityKinds", entity_kinds}, {"visibleOnly", object_payload.value("visibleOnly", true)}}},
        {"selectionIntent",
         {{"operation", replace ? "replace" : "append"}, {"entityKinds", entity_kinds}}},
        {"queryModel",
         {{"interactive", "screen-rectangle"}, {"headlessEquivalent", "camera-frustum-query"}}},
        {"replayBoundary",
         {{"kind", "selection-query"},
          {"recordRawInput", false},
          {"headlessReady", true},
          {"reason", "Restore the camera state first, then replay the normalized box query."}}},
        {"predictedResult",
         {{"selectionCount", static_cast<int>(entity_kinds.size())},
          {"hits", buildPlaceholderHits(entity_kinds)}}}
    };
}

}  // namespace OpenGeoLab::Selection
