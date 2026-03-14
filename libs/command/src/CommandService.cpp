#include <ogl/command/CommandService.hpp>

#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/geometry/GeometryComponentRegistration.hpp>
#include <ogl/render/RenderComponentRegistration.hpp>
#include <ogl/scene/SceneComponentRegistration.hpp>
#include <ogl/selection/SelectionComponentRegistration.hpp>

#include <mutex>
#include <sstream>

namespace {

void ensurePlaceholderServicesRegistered() {
    static std::once_flag once;
    std::call_once(once, []() {
        OGL::Geometry::registerGeometryComponents();
        OGL::Scene::registerSceneComponents();
        OGL::Render::registerRenderComponents();
        OGL::Selection::registerSelectionComponents();
    });
}

auto collectRequests(const std::vector<OGL::Command::CommandRecordEntry>& history)
    -> std::vector<OGL::Command::CommandRequest> {
    std::vector<OGL::Command::CommandRequest> requests;
    requests.reserve(history.size());
    for(const auto& entry : history) {
        requests.push_back(entry.request);
    }
    return requests;
}

} // namespace

namespace OGL::Command {

auto CommandRequest::toJson() const -> nlohmann::json {
    return {{"moduleName", moduleName}, {"params", params}};
}

auto CommandRecordEntry::toJson() const -> nlohmann::json {
    return {{"sequence", sequence}, {"request", request.toJson()}, {"response", response}};
}

auto ReplayReport::toJson() const -> nlohmann::json {
    return {
        {"replayedCount", replayedCount}, {"successCount", successCount}, {"responses", responses}};
}

CommandService::CommandService() { ensurePlaceholderServicesRegistered(); }

auto CommandService::execute(const CommandRequest& request) const -> OGL::Core::ServiceResponse {
    return OGL::Core::ComponentRequestDispatcher::dispatch(request.moduleName, request.params);
}

auto CommandService::exportPythonScript(const std::vector<CommandRequest>& requests)
    -> std::string {
    std::ostringstream script;
    script << "import opengeolab\n\n";
    script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n\n";

    for(std::size_t index = 0; index < requests.size(); ++index) {
        const auto& request = requests[index];
        script << "result_" << (index + 1) << " = bridge.call(\"" << request.moduleName
               << "\", R\"JSON(" << request.params.dump(2) << ")JSON\")\n";
        script << "print(result_" << (index + 1) << ")\n\n";
    }

    if(requests.empty()) {
        script << "# No recorded commands yet.\n";
    }

    return script.str();
}

CommandRecorder::CommandRecorder() = default;

auto CommandRecorder::execute(const CommandRequest& request) -> OGL::Core::ServiceResponse {
    const auto response = m_commandService.execute(request);
    if(response.success) {
        m_history.push_back(
            {.sequence = m_nextSequence++, .request = request, .response = response.toJson()});
    }
    return response;
}

auto CommandRecorder::replayAll() const -> ReplayReport {
    ReplayReport report;
    report.replayedCount = static_cast<int>(m_history.size());

    for(const auto& entry : m_history) {
        const auto response = m_commandService.execute(entry.request);
        report.responses.push_back(response.toJson());
        if(response.success) {
            ++report.successCount;
        }
    }

    return report;
}

void CommandRecorder::clear() {
    m_history.clear();
    m_nextSequence = 1;
}

auto CommandRecorder::recordedCount() const -> int { return static_cast<int>(m_history.size()); }

auto CommandRecorder::history() const -> const std::vector<CommandRecordEntry>& {
    return m_history;
}

auto CommandRecorder::historyJson() const -> nlohmann::json {
    nlohmann::json history_json = nlohmann::json::array();
    for(const auto& entry : m_history) {
        history_json.push_back(entry.toJson());
    }
    return history_json;
}

auto CommandRecorder::exportedPythonScript() const -> std::string {
    return CommandService::exportPythonScript(collectRequests(m_history));
}

} // namespace OGL::Command