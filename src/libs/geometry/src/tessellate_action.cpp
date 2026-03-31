/**
 * @file tessellate_action.cpp
 * @brief TessellateAction — explicit tessellation of a stored shape
 */

#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/tessellate_action.hpp>

namespace OpenGeoLab::Geometry {

TessellateAction::TessellateAction(ShapeStore& store) : m_store(store) {}
TessellateAction::~TessellateAction() = default;

nlohmann::json TessellateAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Tessellate (or re-tessellate) a shape."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Shape identifier to tessellate."}}},
          {"linearDeflection",
           {{"type", "number"},
            {"required", false},
            {"description", "Tessellation linear deflection (default 0.1)"}}},
          {"angularDeflection",
           {{"type", "number"},
            {"required", false},
            {"description", "Tessellation angular deflection (default 0.5)"}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"shapeId", {{"type", "integer"}, {"description", "Tessellated shape identifier."}}}}}};
}

nlohmann::json TessellateAction::execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) {
    const auto shape_id = param.value("shapeId", static_cast<uint32_t>(0));
    const auto tess_params = TessellationParams::fromJson(param);

    if(!m_store.find(shape_id)) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"summary", "Unknown shapeId"}};
    }

    if(progress) {
        progress(0.0, "Tessellating...");
    }
    m_store.tessellate(shape_id, tess_params);
    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "tessellate"}, {"shapeId", shape_id}};
}

} // namespace OpenGeoLab::Geometry
