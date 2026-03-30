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
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"path", {{"type", "string"}, {"description", "Echo of the input file path."}}}}}};
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
    throw std::runtime_error("read_brep: BRep reading is not yet implemented");
}

} // namespace OpenGeoLab::IO
