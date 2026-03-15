/**
 * @file CommandService.hpp
 * @brief High-level command execution, recording, replay, and Python export helpers.
 */

#pragma once

#include <ogl/command/export.hpp>
#include <ogl/core/IService.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace OGL::Command {

struct OGL_COMMAND_EXPORT CommandRequest {
    std::string module;
    std::string action;
    nlohmann::json param = nlohmann::json::object();

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

struct OGL_COMMAND_EXPORT CommandRecordEntry {
    int sequence{0};
    CommandRequest request;
    nlohmann::json response = nlohmann::json::object();

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

struct OGL_COMMAND_EXPORT ReplayReport {
    int replayedCount{0};
    int successCount{0};
    nlohmann::json responses = nlohmann::json::array();

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

class OGL_COMMAND_EXPORT CommandService {
public:
    CommandService();

    [[nodiscard]] auto execute(const CommandRequest& request,
                               const OGL::Core::ProgressCallback& progress_callback = {}) const
        -> OGL::Core::ServiceResponse;

    [[nodiscard]] static auto exportPythonScript(const std::vector<CommandRequest>& requests)
        -> std::string;
};

class OGL_COMMAND_EXPORT CommandRecorder {
public:
    CommandRecorder();

    auto execute(const CommandRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback = {})
        -> OGL::Core::ServiceResponse;
    [[nodiscard]] auto replayAll() const -> ReplayReport;
    void clear();

    [[nodiscard]] auto recordedCount() const -> int;
    [[nodiscard]] auto history() const -> const std::vector<CommandRecordEntry>&;
    [[nodiscard]] auto historyJson() const -> nlohmann::json;
    [[nodiscard]] auto exportedPythonScript() const -> std::string;

private:
    CommandService m_commandService;
    std::vector<CommandRecordEntry> m_history;
    int m_nextSequence{1};
};

} // namespace OGL::Command
