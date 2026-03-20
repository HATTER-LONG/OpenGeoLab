/**
 * @file AppController.hpp
 * @brief Exposes the minimal JSON process bridge to the QML shell.
 */

#pragma once

#include <QObject>
#include <QString>

#include <memory>

namespace OpenGeoLab::Python
{
class EmbeddedPythonRuntime;
}

namespace OpenGeoLab::App
{

/**
 * @brief Bridges the QML shell to the embedded Python runtime.
 */
class AppController: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString requestText READ requestText WRITE setRequestText NOTIFY requestTextChanged)
    Q_PROPERTY(QString responseText READ responseText NOTIFY responseTextChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString snapshotUrl READ snapshotUrl NOTIFY snapshotUrlChanged)

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    [[nodiscard]] QString requestText() const;
    void setRequestText(const QString& request_text);

    [[nodiscard]] QString responseText() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString snapshotUrl() const;

    Q_INVOKABLE void sendRequest();
    Q_INVOKABLE void loadPingExample();
    Q_INVOKABLE void loadGeometryExample();
    Q_INVOKABLE void loadViewportExample();
    Q_INVOKABLE void loadSnapshotExample();
    Q_INVOKABLE void loadSelectionPickExample();
    Q_INVOKABLE void loadBoxSelectionExample();
    Q_INVOKABLE void loadReplayExportExample();
    Q_INVOKABLE void loadPluginExample();

signals:
    void requestTextChanged();
    void responseTextChanged();
    void statusTextChanged();
    void snapshotUrlChanged();

private:
    void setResponseText(const QString& response_text);
    void setStatusText(const QString& status_text);
    void setSnapshotUrl(const QString& snapshot_url);
    void refreshArtifactsFromResponse(const QString& response_text);

    QString m_requestText;
    QString m_responseText;
    QString m_statusText;
    QString m_snapshotUrl;
    std::unique_ptr<OpenGeoLab::Python::EmbeddedPythonRuntime> m_runtime;
};

}  // namespace OpenGeoLab::App
