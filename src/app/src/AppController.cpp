#include <opengeolab/app/AppController.hpp>
#include <opengeolab/python/EmbeddedPythonRuntime.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <stdexcept>

namespace OpenGeoLab::App
{

namespace
{

[[nodiscard]] QString createExampleRequest(const QString& action, const QJsonObject& payload)
{
    const QJsonObject request {
        {"protocolVersion", QStringLiteral("1.0")},
        {"requestId", QStringLiteral("qml-demo")},
        {"source", QStringLiteral("qml-shell")},
        {"action", action},
        {"payload", payload},
        {"context",
         QJsonObject {
             {"llm", QJsonObject {{"requestedBy", QStringLiteral("OpenGeoLab QML shell")}}},
             {"ui", QJsonObject {{"surface", QStringLiteral("main-window")}}}
         }}
    };

    return QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Indented));
}

[[nodiscard]] QString makeDataUrl(const QString& mime_type, const QString& base64_data)
{
    if (mime_type.isEmpty() || base64_data.isEmpty()) {
        return {};
    }

    return QStringLiteral("data:%1;base64,%2").arg(mime_type, base64_data);
}

}  // namespace

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    const QDir application_dir(QCoreApplication::applicationDirPath());
    const auto application_root = std::filesystem::path(application_dir.absolutePath().toStdString());
    const auto runtime_root = application_root / "python";
    const auto plugin_root = application_root / "plugins" / "python";

    loadPingExample();
    try {
        m_runtime = std::make_unique<OpenGeoLab::Python::EmbeddedPythonRuntime>(
            application_root,
            runtime_root,
            plugin_root
        );
        setStatusText(QStringLiteral("Ready"));
    }
    catch (const std::runtime_error& error) {
        setResponseText(QString::fromUtf8(error.what()));
        setStatusText(QStringLiteral("Python runtime initialization failed"));
    }
}

AppController::~AppController() = default;

QString AppController::requestText() const
{
    return m_requestText;
}

void AppController::setRequestText(const QString& request_text)
{
    if (m_requestText == request_text) {
        return;
    }

    m_requestText = request_text;
    emit requestTextChanged();
}

QString AppController::responseText() const
{
    return m_responseText;
}

QString AppController::statusText() const
{
    return m_statusText;
}

QString AppController::snapshotUrl() const
{
    return m_snapshotUrl;
}

void AppController::sendRequest()
{
    if (!m_runtime) {
        setStatusText(QStringLiteral("Python runtime unavailable"));
        return;
    }

    try {
        setStatusText(QStringLiteral("Processing request..."));
        const auto response = QString::fromStdString(m_runtime->process(m_requestText.toStdString()));
        setResponseText(response);
        refreshArtifactsFromResponse(response);
    }
    catch (const std::runtime_error& error) {
        setResponseText(QString::fromUtf8(error.what()));
        setSnapshotUrl({});
        setStatusText(QStringLiteral("Python bridge error"));
    }
}

void AppController::loadPingExample()
{
    setRequestText(createExampleRequest(QStringLiteral("system.ping"), QJsonObject {}));
}

void AppController::loadGeometryExample()
{
    setRequestText(
        createExampleRequest(
            QStringLiteral("geometry.box.describe"),
            QJsonObject {
                {"width", 4.0},
                {"height", 2.5},
                {"depth", 1.5}
            }
        )
    );
}

void AppController::loadSnapshotExample()
{
    setRequestText(
        createExampleRequest(
            QStringLiteral("render.snapshot.capture"),
            QJsonObject {
                {"viewportId", QStringLiteral("mainViewport")},
                {"reason", QStringLiteral("qml-preview")}
            }
        )
    );
}

void AppController::loadPluginExample()
{
    setRequestText(
        createExampleRequest(
            QStringLiteral("plugins.list"),
            QJsonObject {}
        )
    );
}

void AppController::setResponseText(const QString& response_text)
{
    if (m_responseText == response_text) {
        return;
    }

    m_responseText = response_text;
    emit responseTextChanged();
}

void AppController::setStatusText(const QString& status_text)
{
    if (m_statusText == status_text) {
        return;
    }

    m_statusText = status_text;
    emit statusTextChanged();
}

void AppController::setSnapshotUrl(const QString& snapshot_url)
{
    if (m_snapshotUrl == snapshot_url) {
        return;
    }

    m_snapshotUrl = snapshot_url;
    emit snapshotUrlChanged();
}

void AppController::refreshArtifactsFromResponse(const QString& response_text)
{
    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(response_text.toUtf8(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        setSnapshotUrl({});
        setStatusText(QStringLiteral("Response received (non-JSON)"));
        return;
    }

    const QJsonObject response = document.object();
    setStatusText(response.value(QStringLiteral("summary")).toString(QStringLiteral("Response received")));

    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    const QJsonObject snapshot_container = result.value(QStringLiteral("snapshot")).toObject();
    if (!snapshot_container.isEmpty()) {
        setSnapshotUrl(
            makeDataUrl(
                snapshot_container.value(QStringLiteral("mimeType")).toString(),
                snapshot_container.value(QStringLiteral("data")).toString()
            )
        );
        return;
    }

    setSnapshotUrl({});
}

}  // namespace OpenGeoLab::App
