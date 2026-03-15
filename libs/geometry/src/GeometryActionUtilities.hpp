#pragma once

#include <ogl/geometry/GeometryLogger.hpp>
#include <ogl/geometry/GeometryModel.hpp>

#include <algorithm>
#include <sstream>
#include <string>

namespace OGL::Geometry::Internal {

struct GeometryCreateSpec {
    std::string action;
    std::string shapeType;
    std::string defaultModelName;
};

inline auto requestObject(const nlohmann::json& value, const char* key) -> nlohmann::json {
    if(value.contains(key) && value.at(key).is_object()) {
        return value.at(key);
    }
    return nlohmann::json::object();
}

inline auto vectorValue(const nlohmann::json& object, const char* key, double fallback) -> double {
    if(object.contains(key) && object.at(key).is_number()) {
        return object.at(key).get<double>();
    }
    return fallback;
}

inline auto buildDescriptor(const nlohmann::json& param)
    -> OGL::Geometry::GeometryDescriptor {
    return {
        .modelName = param.value("modelName", std::string{"Bracket_A01"}),
        .bodyCount = param.value("bodyCount", 3),
        .source = param.value("source", std::string{"component"}),
    };
}

inline auto normalizeInspectParam(const nlohmann::json& param) -> nlohmann::json {
    return {
        {"modelName", param.value("modelName", std::string{"Bracket_A01"})},
        {"bodyCount", std::max(param.value("bodyCount", 3), 1)},
        {"source", param.value("source", std::string{"component"})},
    };
}

inline auto normalizeBoxParam(const GeometryCreateSpec& create_spec, const nlohmann::json& param)
    -> nlohmann::json {
    const auto origin = requestObject(param, "origin");
    const auto dimensions = requestObject(param, "dimensions");
    return {
        {"modelName", param.value("modelName", create_spec.defaultModelName)},
        {"bodyCount", 1},
        {"source", param.value("source", std::string{"component"})},
        {"origin",
         {
             {"x", vectorValue(origin, "x", param.value("originX", 0.0))},
             {"y", vectorValue(origin, "y", param.value("originY", 0.0))},
             {"z", vectorValue(origin, "z", param.value("originZ", 0.0))},
         }},
        {"dimensions",
         {
             {"x", vectorValue(dimensions, "x", param.value("width", param.value("sizeX", 10.0)))},
             {"y", vectorValue(dimensions, "y", param.value("height", param.value("sizeY", 10.0)))},
             {"z", vectorValue(dimensions, "z", param.value("depth", param.value("sizeZ", 10.0)))},
         }},
    };
}

inline auto normalizeCylinderParam(const GeometryCreateSpec& create_spec,
                                   const nlohmann::json& param) -> nlohmann::json {
    const auto base_center = requestObject(param, "baseCenter");
    return {
        {"modelName", param.value("modelName", create_spec.defaultModelName)},
        {"bodyCount", 1},
        {"source", param.value("source", std::string{"component"})},
        {"baseCenter",
         {
             {"x", vectorValue(base_center, "x", param.value("x", param.value("originX", 0.0)))},
             {"y", vectorValue(base_center, "y", param.value("y", param.value("originY", 0.0)))},
             {"z", vectorValue(base_center, "z", param.value("z", param.value("originZ", 0.0)))},
         }},
        {"radius", param.value("radius", 5.0)},
        {"height", param.value("height", 10.0)},
        {"axis", param.value("axis", std::string{"Z"})},
    };
}

inline auto normalizeSphereParam(const GeometryCreateSpec& create_spec, const nlohmann::json& param)
    -> nlohmann::json {
    const auto center = requestObject(param, "center");
    return {
        {"modelName", param.value("modelName", create_spec.defaultModelName)},
        {"bodyCount", 1},
        {"source", param.value("source", std::string{"component"})},
        {"center",
         {
             {"x", vectorValue(center, "x", param.value("x", param.value("originX", 0.0)))},
             {"y", vectorValue(center, "y", param.value("y", param.value("originY", 0.0)))},
             {"z", vectorValue(center, "z", param.value("z", param.value("originZ", 0.0)))},
         }},
        {"radius", param.value("radius", 5.0)},
    };
}

inline auto normalizeTorusParam(const GeometryCreateSpec& create_spec, const nlohmann::json& param)
    -> nlohmann::json {
    const auto center = requestObject(param, "center");
    return {
        {"modelName", param.value("modelName", create_spec.defaultModelName)},
        {"bodyCount", 1},
        {"source", param.value("source", std::string{"component"})},
        {"center",
         {
             {"x", vectorValue(center, "x", param.value("x", param.value("originX", 0.0)))},
             {"y", vectorValue(center, "y", param.value("y", param.value("originY", 0.0)))},
             {"z", vectorValue(center, "z", param.value("z", param.value("originZ", 0.0)))},
         }},
        {"majorRadius", param.value("majorRadius", 10.0)},
        {"minorRadius", param.value("minorRadius", 3.0)},
        {"axis", param.value("axis", std::string{"Z"})},
    };
}

inline auto buildEquivalentPython(const OGL::Core::ServiceRequest& request) -> std::string {
    std::ostringstream script;
    script << "import json\n";
    script << "import opengeolab\n\n";
    script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
    script << "request = json.loads(r'''" << request.toJson().dump(2) << "''')\n";
    script << "result = bridge.process(request)\n";
    script << "print(result)";
    return script.str();
}

inline auto buildCreateSummary(const GeometryCreateSpec& create_spec,
                               const OGL::Geometry::GeometryModel& model)
    -> std::string {
    return create_spec.shapeType + " request accepted for '" + model.modelName() + "' via " +
           model.source() + ".";
}

inline auto buildCreateResponse(const OGL::Core::ServiceRequest& request,
                                const GeometryCreateSpec& create_spec,
                                const nlohmann::json& normalized_param,
                                const OGL::Geometry::GeometryModel& model)
    -> OGL::Core::ServiceResponse {
    return {
        .success = true,
        .module = request.module,
        .action = request.action,
        .message = "Geometry request completed through the modular component pipeline.",
        .payload =
            {
                {"shapeType", create_spec.shapeType},
                {"model", model.toJson()},
                {"summary", buildCreateSummary(create_spec, model)},
                {"equivalentPython", buildEquivalentPython({.module = request.module,
                                                            .action = request.action,
                                                            .param = normalized_param})},
            },
    };
}

inline auto buildInspectResponse(const OGL::Core::ServiceRequest& request,
                                 const nlohmann::json& normalized_param,
                                 const OGL::Geometry::GeometryModel& model)
    -> OGL::Core::ServiceResponse {
    return {
        .success = true,
        .module = request.module,
        .action = request.action,
        .message = "Geometry request completed through the modular component pipeline.",
        .payload =
            {
                {"model", model.toJson()},
                {"summary", model.summary()},
                {"equivalentPython", buildEquivalentPython({.module = request.module,
                                                            .action = request.action,
                                                            .param = normalized_param})},
            },
    };
}

inline auto cancellationResponse(const OGL::Core::ServiceRequest& request,
                                 const std::string& message) -> OGL::Core::ServiceResponse {
    return {.success = false,
            .module = request.module,
            .action = request.action,
            .message = message,
            .payload = nlohmann::json::object()};
}

inline auto reportProgress(const OGL::Core::ProgressCallback& progress_callback,
                           double progress,
                           const std::string& message) -> bool {
    return !progress_callback || progress_callback(progress, message);
}

} // namespace OGL::Geometry::Internal

