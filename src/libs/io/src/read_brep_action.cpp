/**
 * @file read_brep_action.cpp
 * @brief ReadBrepAction implementation
 */

#include <opengeolab/io/read_brep_action.hpp>

#include <opengeolab/core/logger.hpp>

#include <stdexcept>

namespace OpenGeoLab::IO {

ReadBrepAction::ReadBrepAction() = default;
ReadBrepAction::~ReadBrepAction() = default;

nlohmann::json ReadBrepAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Read a BRep (Boundary Representation) geometry file and return its data."},
        {"params",
         {{"path",
           {{"type", "string"},
            {"required", true},
            {"description", "File system path to the BRep file to read"}}}}},
        {"returns",
         {{"status", {{"type", "string"}, {"description", "\"ok\" on success"}}},
          {"action",
           {{"type", "string"}, {"description", "Echo of the action name (\"read_brep\")"}}},
          {"path", {{"type", "string"}, {"description", "Echo of the input file path"}}}}}};
}

nlohmann::json ReadBrepAction::execute(const nlohmann::json& param,
                                       const Core::ProgressCallback& progress) {
    if(!param.contains("path") || !param["path"].is_string()) {
        throw std::invalid_argument("read_brep: \"path\" string parameter is required");
    }

    const auto path = param["path"].get<std::string>();
    LOG_INFO("ReadBrepAction: reading BRep from '{}'", path);

    progress(0.0, "Reading BRep file...");

    // TODO(layton): Implement actual BRep file reading logic
    nlohmann::json result;
    result["status"] = "ok";
    result["action"] = "read_brep";
    result["path"] = path;

    progress(1.0, "BRep file read complete");
    return result;
}

} // namespace OpenGeoLab::IO
