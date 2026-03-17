#include "OpenGeoLabAutomationFacade.hpp"

#include <ogl/app/EmbeddedPythonRuntime.hpp>

#include <QSaveFile>
#include <QTextStream>
#include <QUrl>

namespace OGL::App {

namespace {

auto formatPythonOutput(const std::string& output, const char* emptyMessage) -> QString {
    return QString::fromStdString(output.empty() ? std::string{emptyMessage} : output);
}

auto resolveExportPath(const QString& filePath) -> QString {
    const QUrl url(filePath);
    if(url.isLocalFile()) {
        return url.toLocalFile();
    }
    return filePath;
}

} // namespace

OpenGeoLabAutomationFacade::OpenGeoLabAutomationFacade(
    EmbeddedPythonRuntime::ProcessRequestHandler processRequest)
    : m_embeddedPythonRuntime(std::make_unique<EmbeddedPythonRuntime>(std::move(processRequest))) {}

OpenGeoLabAutomationFacade::~OpenGeoLabAutomationFacade() = default;

auto OpenGeoLabAutomationFacade::runEmbeddedPython(const QString& script) -> AutomationResult {
    if(script.trimmed().isEmpty()) {
        const QString message = QStringLiteral("Python script is empty.");
        return {.success = false,
                .notice = {.level = 4,
                           .source = QStringLiteral("python.executeScript"),
                           .message = message,
                           .detail = QString()},
                .pythonOutput = PythonOutputEvent{.outputText = message}};
    }

    try {
        const QString output =
            formatPythonOutput(m_embeddedPythonRuntime->executeScript(script.toStdString()),
                               "Python script completed without stdout/stderr.");
        return {.success = true,
                .notice = {.level = 2,
                           .source = QStringLiteral("python.executeScript"),
                           .message = QStringLiteral("Embedded Python script completed."),
                           .detail = output},
                .pythonOutput = PythonOutputEvent{.outputText = output}};
    } catch(const std::exception& ex) {
        const QString output = QString::fromStdString(ex.what());
        return {.success = false,
                .notice = {.level = 4,
                           .source = QStringLiteral("python.executeScript"),
                           .message = QStringLiteral("Embedded Python script failed."),
                           .detail = output},
                .pythonOutput = PythonOutputEvent{.outputText = output}};
    }
}

auto OpenGeoLabAutomationFacade::runEmbeddedPythonCommandLine(const QString& commandLine)
    -> AutomationResult {
    if(commandLine.trimmed().isEmpty()) {
        const QString message = QStringLiteral("Python command line is empty.");
        return {.success = false,
                .notice = {.level = 4,
                           .source = QStringLiteral("python.commandLine"),
                           .message = message,
                           .detail = QString()},
                .pythonOutput = PythonOutputEvent{.outputText = message}};
    }

    try {
        const QString output = formatPythonOutput(
            m_embeddedPythonRuntime->executeCommandLine(commandLine.toStdString()),
            "Python command completed without stdout/stderr.");
        return {.success = true,
                .notice = {.level = 2,
                           .source = QStringLiteral("python.commandLine"),
                           .message = QStringLiteral("Python command line completed."),
                           .detail = output},
                .pythonOutput = PythonOutputEvent{.outputText = output}};
    } catch(const std::exception& ex) {
        const QString output = QString::fromStdString(ex.what());
        return {.success = false,
                .notice = {.level = 4,
                           .source = QStringLiteral("python.commandLine"),
                           .message = QStringLiteral("Python command line failed."),
                           .detail = output},
                .pythonOutput = PythonOutputEvent{.outputText = output}};
    }
}

auto OpenGeoLabAutomationFacade::exportRecordedScript(const QString& filePath,
                                                      const QString& recordedScript,
                                                      const QString& suggestedPython,
                                                      int recordedCommandCount)
    -> AutomationResult {
    const QString targetPath = resolveExportPath(filePath).trimmed();
    const QString exportedScript =
        recordedCommandCount > 0
            ? recordedScript
            : (suggestedPython.trimmed().isEmpty() ? recordedScript : suggestedPython);

    if(targetPath.isEmpty()) {
        const QString message = QStringLiteral("Export path is empty.");
        return {.success = false,
                .notice = {.level = 4,
                           .source = QStringLiteral("python.exportRecordedScript"),
                           .message = message,
                           .detail = QString()},
                .pythonOutput = PythonOutputEvent{.outputText = message}};
    }

    if(exportedScript.trimmed().isEmpty()) {
        const QString message = QStringLiteral("No recorded Python script is available to export.");
        return {.success = false,
                .notice = {.level = 4,
                           .source = QStringLiteral("python.exportRecordedScript"),
                           .message = message,
                           .detail = QString()},
                .pythonOutput = PythonOutputEvent{.outputText = message}};
    }

    QSaveFile outputFile(targetPath);
    if(!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString message =
            QStringLiteral("Failed to open export file: %1").arg(outputFile.errorString());
        return {.success = false,
                .notice = {.level = 4,
                           .source = QStringLiteral("python.exportRecordedScript"),
                           .message = message,
                           .detail = QString()},
                .pythonOutput = PythonOutputEvent{.outputText = message}};
    }

    QTextStream outputStream(&outputFile);
    outputStream << exportedScript;
    if(!exportedScript.endsWith('\n')) {
        outputStream << '\n';
    }

    if(!outputFile.commit()) {
        const QString message =
            QStringLiteral("Failed to save export file: %1").arg(outputFile.errorString());
        return {.success = false,
                .notice = {.level = 4,
                           .source = QStringLiteral("python.exportRecordedScript"),
                           .message = message,
                           .detail = QString()},
                .pythonOutput = PythonOutputEvent{.outputText = message}};
    }

    const QString summary = QStringLiteral("Recorded Python script exported to %1").arg(targetPath);
    return {.success = true,
            .notice = {.level = 2,
                       .source = QStringLiteral("python.exportRecordedScript"),
                       .message = summary,
                       .detail = QString()},
            .pythonOutput = PythonOutputEvent{.outputText = summary}};
}

} // namespace OGL::App
