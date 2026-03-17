/**
 * @file OpenGeoLabAutomationFacade.hpp
 * @brief Handles embedded Python execution and Python script export for the app controller.
 */

#pragma once

#include "OpenGeoLabControllerEvents.hpp"

#include <ogl/app/EmbeddedPythonRuntime.hpp>

#include <memory>

#include <nlohmann/json.hpp>

namespace OGL::App {

struct AutomationResult {
    bool success{false};
    UiNotice notice;
    std::optional<PythonOutputEvent> pythonOutput;
};

class OpenGeoLabAutomationFacade {
public:
    explicit OpenGeoLabAutomationFacade(
        EmbeddedPythonRuntime::ProcessRequestHandler processRequest);
    ~OpenGeoLabAutomationFacade();

    OpenGeoLabAutomationFacade(const OpenGeoLabAutomationFacade&) = delete;
    auto operator=(const OpenGeoLabAutomationFacade&) -> OpenGeoLabAutomationFacade& = delete;
    OpenGeoLabAutomationFacade(OpenGeoLabAutomationFacade&&) = delete;
    auto operator=(OpenGeoLabAutomationFacade&&) -> OpenGeoLabAutomationFacade& = delete;

    auto runEmbeddedPython(const QString& script) -> AutomationResult;
    auto runEmbeddedPythonCommandLine(const QString& commandLine) -> AutomationResult;
    auto exportRecordedScript(const QString& filePath,
                              const QString& recordedScript,
                              const QString& suggestedPython,
                              int recordedCommandCount) -> AutomationResult;

private:
    std::unique_ptr<EmbeddedPythonRuntime> m_embeddedPythonRuntime;
};

} // namespace OGL::App
