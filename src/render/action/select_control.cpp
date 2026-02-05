/**
 * @file select_control.cpp
 * @brief Implementation of SelectControl render action
 */

#include "select_control.hpp"

#include "geometry/geometry_types.hpp"
#include "render/select_manager.hpp"

#include <stdexcept>

namespace OpenGeoLab::Render {

namespace {
[[nodiscard]] SelectManager::PickTypes pickTypesFromString(std::string_view s) {
    if(s == "vertex") {
        return SelectManager::PickTypes::Vertex;
    }
    if(s == "edge") {
        return SelectManager::PickTypes::Edge;
    }
    if(s == "face") {
        return SelectManager::PickTypes::Face;
    }
    if(s == "solid") {
        return SelectManager::PickTypes::Solid;
    }
    if(s == "part") {
        return SelectManager::PickTypes::Part;
    }
    throw std::invalid_argument("Unsupported pick type string");
}

[[nodiscard]] Geometry::EntityType entityTypeFromJson(const nlohmann::json& j) {
    if(j.is_number_integer()) {
        return static_cast<Geometry::EntityType>(j.get<int>());
    }
    if(j.is_string()) {
        return Geometry::entityTypeFromString(j.get<std::string>());
    }
    throw std::invalid_argument("Invalid entity type");
}
} // namespace

nlohmann::json SelectControl::execute(const nlohmann::json& params,
                                      Util::ProgressCallback /*progress_callback*/) {
    if(!params.is_object() || !params.contains("select_ctrl") ||
       !params["select_ctrl"].is_object()) {
        throw std::runtime_error("Missing or invalid 'select_ctrl' parameter.");
    }

    const auto& ctrl = params["select_ctrl"];
    auto& select_manager = SelectManager::instance();

    if(ctrl.contains("enabled") && ctrl["enabled"].is_boolean()) {
        select_manager.setPickEnabled(ctrl["enabled"].get<bool>());
    }

    if(ctrl.contains("types")) {
        const auto& types = ctrl["types"];
        if(types.is_number_integer()) {
            const auto mask = static_cast<uint8_t>(types.get<int>());
            select_manager.setPickTypes(static_cast<SelectManager::PickTypes>(mask));
        } else if(types.is_array()) {
            SelectManager::PickTypes mask = SelectManager::PickTypes::None;
            for(const auto& t : types) {
                if(!t.is_string()) {
                    throw std::invalid_argument("select_ctrl.types array must contain strings");
                }
                mask |= pickTypesFromString(t.get<std::string>());
            }
            select_manager.setPickTypes(mask);
        } else if(types.is_string()) {
            select_manager.setPickTypes(pickTypesFromString(types.get<std::string>()));
        } else {
            throw std::invalid_argument("Invalid select_ctrl.types");
        }
    }

    if(ctrl.contains("clear") && ctrl["clear"].is_boolean() && ctrl["clear"].get<bool>()) {
        select_manager.clearSelections();
    }

    if(ctrl.contains("add") && ctrl["add"].is_object()) {
        const auto& a = ctrl["add"];
        if(!a.contains("type") || !a.contains("uid")) {
            throw std::invalid_argument("select_ctrl.add requires {type, uid}");
        }
        const auto type = entityTypeFromJson(a["type"]);
        const auto uid = static_cast<Geometry::EntityUID>(a["uid"].get<uint64_t>());
        select_manager.addSelection(type, uid);
    }

    if(ctrl.contains("remove") && ctrl["remove"].is_object()) {
        const auto& r = ctrl["remove"];
        if(!r.contains("type") || !r.contains("uid")) {
            throw std::invalid_argument("select_ctrl.remove requires {type, uid}");
        }
        const auto type = entityTypeFromJson(r["type"]);
        const auto uid = static_cast<Geometry::EntityUID>(r["uid"].get<uint64_t>());
        select_manager.removeSelection(type, uid);
    }

    nlohmann::json response{{"status", "success"},
                            {"action", params.value("action", "SelectControl")}};

    const bool want_get =
        ctrl.contains("get") && ctrl["get"].is_boolean() && ctrl["get"].get<bool>();
    if(want_get) {
        nlohmann::json arr = nlohmann::json::array();
        for(const auto& s : select_manager.selections()) {
            arr.push_back(nlohmann::json{{"type", Geometry::entityTypeToString(s.m_type)},
                                         {"uid", static_cast<uint64_t>(s.m_uid)}});
        }
        response["pick_enabled"] = select_manager.isPickEnabled();
        response["pick_types"] = static_cast<int>(select_manager.pickTypes());
        response["selections"] = std::move(arr);
    }

    return response;
}

} // namespace OpenGeoLab::Render
