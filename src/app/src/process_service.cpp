#include <opengeolab/app/process_service.hpp>

#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent>

#include <exception>

namespace OpenGeoLab::App {

ProcessService::ProcessService(OpenGeoLab::Python::EmbeddedPythonRuntime& runtime, QObject* parent)
    : QObject(parent), m_runtime(runtime) {}

bool ProcessService::isBusy() const { return m_pendingCount.load(std::memory_order_relaxed) > 0; }

void ProcessService::submitRequest(const QString& request_json) {
    // Record hook: if (m_recorder) m_recorder->onRequest(requestJson);

    const auto doc = QJsonDocument::fromJson(request_json.toUtf8());
    const auto obj = doc.object();
    const QString request_id = obj.value("requestId").toString("unknown");
    const QString action = obj.value("action").toString();

    // UI-invoking actions MUST run on the main thread because Qt widgets can
    // only be created and operated from the GUI thread.  The Python code will
    // typically show a modal dialog (QDialog.exec / QMessageBox.exec) that
    // runs a nested event loop, keeping the UI responsive while blocked.
    if(action.startsWith("plugins.invoke_ui")) {
        try {
            auto response =
                QString::fromStdString(m_runtime.process(request_json.toStdString()));
            const auto response_doc = QJsonDocument::fromJson(response.toUtf8());
            if(response_doc.object().value("ok").toBool(false)) {
                emit responseReady(request_id, response);
            } else {
                const QString summary =
                    response_doc.object().value("summary").toString("Unknown error");
                emit errorOccurred(request_id, summary);
            }
        } catch(const std::exception& ex) {
            emit errorOccurred(request_id, QString::fromStdString(ex.what()));
        }
        return;
    }

    // Normal async path for non-UI actions.
    m_pendingCount.fetch_add(1, std::memory_order_relaxed);
    emit busyChanged();

    auto* watcher = new QFutureWatcher<QString>(this);

    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, request_id]() {
        m_pendingCount.fetch_sub(1, std::memory_order_relaxed);
        emit busyChanged();

        try {
            const QString response = watcher->result();
            const auto response_doc = QJsonDocument::fromJson(response.toUtf8());
            if(response_doc.object().value("ok").toBool(false)) {
                emit responseReady(request_id, response);
            } else {
                const QString summary =
                    response_doc.object().value("summary").toString("Unknown error");
                emit errorOccurred(request_id, summary);
            }
        } catch(const std::exception& ex) {
            emit errorOccurred(request_id, QString::fromStdString(ex.what()));
        }

        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([this, json = request_json.toStdString()]() -> QString {
        return QString::fromStdString(m_runtime.process(json));
    }));
}

} // namespace OpenGeoLab::App
