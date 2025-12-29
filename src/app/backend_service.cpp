#include <app/backend_service.hpp>

#include <algorithm>

namespace OpenGeoLab {

BackendService::BackendService(QObject* parent) : QObject(parent) {}

bool BackendService::busy() const { return m_busy; }

double BackendService::progress() const { return m_progress; }

QString BackendService::message() const { return m_message; }

QString BackendService::lastError() const { return m_lastError; }

void BackendService::request(const QString& action_id, const QVariantMap& params) {
    clearError();

    if(action_id.trimmed().isEmpty()) {
        setLastError(QStringLiteral("action_id is empty"));
        emit operationFailed(action_id, m_lastError);
        return;
    }

    if(m_busy) {
        const auto error = QStringLiteral("BackendService is busy");
        setLastError(error);
        emit operationFailed(action_id, error);
        return;
    }

    m_currentActionId = action_id;
    m_currentParams = params;

    setBusyInternal(true);
    setProgressInternal(0.0);
    setMessage(QStringLiteral("%1...").arg(action_id));

    emit operationStarted(action_id);

    // Stub async job: simulate progress without blocking UI.
    m_tick = 0;
    m_totalTicks = 25;

    m_timer = new QTimer(this);
    m_timer->setInterval(80);

    connect(m_timer, &QTimer::timeout, this, [this]() {
        if(!m_timer) {
            return;
        }

        m_tick = std::min(m_tick + 1, m_totalTicks);
        const double p = static_cast<double>(m_tick) / static_cast<double>(m_totalTicks);

        setProgressInternal(p);
        setMessage(
            QStringLiteral("%1 (%2%)").arg(m_currentActionId).arg(static_cast<int>(p * 100.0)));

        emit operationProgress(m_currentActionId, m_progress, m_message);

        if(m_tick >= m_totalTicks) {
            m_timer->stop();
            m_timer->deleteLater();
            m_timer.clear();

            QVariantMap result;
            result.insert(QStringLiteral("actionId"), m_currentActionId);
            result.insert(QStringLiteral("stub"), true);
            result.insert(QStringLiteral("params"), m_currentParams);

            setBusyInternal(false);
            setProgressInternal(1.0);
            setMessage(QString());

            emit operationFinished(m_currentActionId, result);

            m_currentActionId.clear();
            m_currentParams.clear();
        }
    });

    m_timer->start();
}

void BackendService::setBusy(bool busy, const QString& message) {
    setBusyInternal(busy);
    if(!message.isEmpty()) {
        setMessage(message);
    }

    if(!busy) {
        // When cleared manually, also clear any running timer.
        if(m_timer) {
            m_timer->stop();
            m_timer->deleteLater();
            m_timer.clear();
        }
        m_currentActionId.clear();
        m_currentParams.clear();
    }
}

void BackendService::setProgress(double progress, const QString& message) {
    setProgressInternal(progress);
    if(!message.isEmpty()) {
        setMessage(message);
    }
}

void BackendService::clearError() {
    if(m_lastError.isEmpty()) {
        return;
    }
    m_lastError.clear();
    emit lastErrorChanged();
}

void BackendService::cancel() {
    if(!m_busy) {
        return;
    }

    const auto action_id = m_currentActionId;

    if(m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer.clear();
    }

    setBusyInternal(false);
    setProgressInternal(-1.0);
    setMessage(QString());

    const auto error = QStringLiteral("Canceled");
    setLastError(error);

    emit operationFailed(action_id, error);

    m_currentActionId.clear();
    m_currentParams.clear();
}

void BackendService::setLastError(const QString& error) {
    if(m_lastError == error) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged();
}

void BackendService::setMessage(const QString& message) {
    if(m_message == message) {
        return;
    }
    m_message = message;
    emit messageChanged();
}

void BackendService::setProgressInternal(double progress) {
    if(m_progress == progress) {
        return;
    }
    m_progress = progress;
    emit progressChanged();
}

void BackendService::setBusyInternal(bool busy) {
    if(m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

} // namespace OpenGeoLab
