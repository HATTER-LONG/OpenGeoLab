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
    return {{"name", ACTION_NAME},
            {"description", "Tessellate (or re-tessellate) a shape."},
            {"params",
             {{"shapeId", {{"type", "integer"}, {"required", true}}},
              {"linearDeflection", {{"type", "number"}, {"required", false}}},
              {"angularDeflection", {{"type", "number"}, {"required", false}}}}}};
}

nlohmann::json TessellateAction::execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) {
    const auto shape_id = param.value("shapeId", static_cast<uint32_t>(0));
    const auto tess_params = TessellationParams::fromJson(param);

    if(!m_store.find(shape_id)) {
        return {{"ok", false}, {"summary", "Unknown shapeId"}};
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
