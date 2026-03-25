/**
 * @file geometry_module.cpp
 * @brief Minimal geometry module implementation stub.
 */

#include <opengeolab/geometry/geometry_module.hpp>

#include <nlohmann/json.hpp>

namespace OpenGeoLab::Geometry {

std::string GeometryModule::process(const std::string& request) {
    auto req = nlohmann::json::parse(request, nullptr, false);
    if(req.is_discarded()) {
        return R"({"ok":false,"error":"invalid JSON"})";
    }
    return R"({"ok":false,"error":"not implemented"})";
}

}  // namespace OpenGeoLab::Geometry
