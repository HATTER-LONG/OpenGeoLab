#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>

#include <QtQml/qqml.h>

#include <QString>

namespace OpenGeoLab {

class BackendService final : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    // progress: [0,1] for known progress; <0 for indeterminate.
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit BackendService(QObject* parent = nullptr);

    [[nodiscard]] bool busy() const;
    [[nodiscard]] double progress() const;
    [[nodiscard]] QString message() const;
    [[nodiscard]] QString lastError() const;

    // Generic request entry point. This is a stubbed async interface.
    Q_INVOKABLE void request(const QString& action_id, const QVariantMap& params);

    // Optional helpers so QML can drive progress directly for QML-only work.
    Q_INVOKABLE void setBusy(bool busy, const QString& message = QString());
    Q_INVOKABLE void setProgress(double progress, const QString& message = QString());
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void progressChanged();
    void messageChanged();
    void lastErrorChanged();

    void operationStarted(const QString& action_id);
    void operationProgress(const QString& action_id, double progress, const QString& message);
    void operationFinished(const QString& action_id, const QVariantMap& result);
    void operationFailed(const QString& action_id, const QString& error);

private:
    void setLastError(const QString& error);
    void setMessage(const QString& message);
    void setProgressInternal(double progress);
    void setBusyInternal(bool busy);

    bool m_busy = false;
    double m_progress = -1.0;
    QString m_message;
    QString m_lastError;

    QString m_currentActionId;
    QVariantMap m_currentParams;

    QPointer<QTimer> m_timer;
    int m_tick = 0;
    int m_totalTicks = 0;
};

} // namespace OpenGeoLab
