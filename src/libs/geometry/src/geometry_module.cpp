#include <opengeolab/geometry/geometry_module.hpp>

#include <opengeolab/geometry/create_box_action.hpp>

#include <opengeolab/base/notification_registry.hpp>
#include <opengeolab/base/notification_sink.hpp>

#include <nlohmann/json.hpp>

#include <format>

namespace OpenGeoLab::Geometry {

namespace {

void notifyIfAvailable(std::string_view channel, const std::string& payload) {
    auto* sink = OpenGeoLab::Base::NotificationRegistry::sink();
    if(sink != nullptr) {
        sink->notify(channel, payload);
    }
}

void attachRequestId(nlohmann::json& response, const std::string& request_id) {
    if(!request_id.empty()) {
        response["requestId"] = request_id;
    }
}

nlohmann::json boxToJson(int id, const BoxData& box) {
    nlohmann::json result;
    result["id"] = id;
    result["label"] = box.label;
    result["center"] = box.center;
    result["size"] = box.size;
    result["vertexCount"] = box.vertexCount;
    return result;
}

} // namespace

GeometryModule::GeometryModule(SceneStore& store) : m_store(store) {}

std::string GeometryModule::process(std::string_view request_json,
                                    const ModuleProgressCallback& progress_callback) {
    const std::lock_guard lock(m_processMutex);
    try {
        const auto request = nlohmann::json::parse(request_json);
        const auto action = request.value("action", "");
        const auto request_id = request.value("requestId", "");

        if(action == "create_box") {
            const auto param = request.value("param", nlohmann::json::object());
            const auto vertex_count = param.value("vertexCount", 100);

            const auto center_array = param.value("center", std::array<double, 3>{0.0, 0.0, 0.0});
            const auto size_array = param.value("size", std::array<double, 3>{1.0, 1.0, 1.0});

            const auto box = createBox(center_array, size_array, vertex_count, progress_callback);
            const int id = m_store.addBox(box);

            notifyIfAvailable(
                "geometry.status",
                std::format(R"({{"event":"completed","action":"create_box","label":"{}"}})",
                            box.label));
            notifyIfAvailable(
                "geometry.data_changed",
                std::format(R"({{"event":"data_changed","count":{}}})", m_store.boxCount()));

            nlohmann::json response;
            response["ok"] = true;
            response["module"] = "geometry";
            response["action"] = "create_box";
            response["summary"] = std::format("Created {}", box.label);
            response["result"] = boxToJson(id, box);
            attachRequestId(response, request_id);
            return response.dump();
        }

        if(action == "list_boxes") {
            const auto boxes = m_store.allBoxes();

            nlohmann::json serialized_boxes = nlohmann::json::array();
            for(const auto& [id, box] : boxes) {
                serialized_boxes.push_back(boxToJson(id, box));
            }

            nlohmann::json response;
            response["ok"] = true;
            response["module"] = "geometry";
            response["action"] = "list_boxes";
            response["result"] = {{"boxes", serialized_boxes}, {"count", boxes.size()}};
            attachRequestId(response, request_id);
            return response.dump();
        }

        nlohmann::json response;
        response["ok"] = false;
        response["module"] = "geometry";
        response["action"] = action;
        response["summary"] = std::format("Unknown geometry action: {}", action);
        attachRequestId(response, request_id);
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
