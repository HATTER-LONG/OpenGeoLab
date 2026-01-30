#include "create_action.hpp"

namespace OpenGeoLab::Geometry {
[[nodiscard]] bool CreateAction::execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) {
    if(!params.contains("type")) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing 'type' parameter.");
        }
        return false;
    }
    std::string type = params["type"];
    if(type == "box") {
        // Create box
    } else {
        if(progress_callback) {
            progress_callback(1.0, "Error: Unsupported shape type '" + type + "'.");
        }
        return false;
    }

    return true;
}

} // namespace OpenGeoLab::Geometry