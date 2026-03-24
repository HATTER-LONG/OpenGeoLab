#include <opengeolab/geometry/geometry_module.hpp>

#include <opengeolab/geometry/create_box_action.hpp>

#include <nlohmann/json.hpp>

#include <format>

namespace OpenGeoLab::Geometry {

std::string processGeometry(std::string_view request_json,
                            ModuleProgressCallback progress_callback) {
    try {
        const auto request = nlohmann::json::parse(request_json);
        const auto action = request.value("action", "");
        const auto request_id = request.value("requestId", "");

        if(action == "create_box") {
            const auto param = request.value("param", nlohmann::json::object());
            const auto vertex_count = param.value("vertexCount", 100);

            const auto center_array = param.value("center", std::array<double, 3>{0.0, 0.0, 0.0});
            const auto size_array = param.value("size", std::array<double, 3>{1.0, 1.0, 1.0});

            ProgressCallback geometry_callback;
            if(progress_callback) {
                geometry_callback = [&progress_callback](double progress,
                                                         std::string_view message) {
                    progress_callback(progress, message);
                };
            }

            const auto box =
                createBox(center_array, size_array, vertex_count, std::move(geometry_callback));

            nlohmann::json result;
            result["center"] = box.center;
            result["size"] = box.size;
            result["vertexCount"] = box.vertexCount;
            result["label"] = box.label;

            nlohmann::json response;
            response["ok"] = true;
            response["module"] = "geometry";
            response["action"] = "create_box";
            response["summary"] = std::format("Created {}", box.label);
            response["result"] = result;
            if(!request_id.empty()) {
                response["request_id"] = request_id;
            }
            return response.dump();
        }

        nlohmann::json response;
        response["ok"] = false;
        response["module"] = "geometry";
        response["action"] = action;
        response["summary"] = std::format("Unknown geometry action: {}", action);
        if(!request_id.empty()) {
            response["request_id"] = request_id;
        }
        return response.dump();

    } catch(const std::exception& e) {
        nlohmann::json response;
        response["ok"] = false;
        response["module"] = "geometry";
        response["action"] = "unknown";
        response["summary"] = std::format("Geometry module error: {}", e.what());
        return response.dump();
    }
}

} // namespace OpenGeoLab::Geometry
