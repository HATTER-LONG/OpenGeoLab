#include <app/log_service.hpp>

#include <QMetaObject>
#include <QThread>

namespace OpenGeoLab::App {

LogService::LogService(QObject* parent) : QObject(parent), m_model(this) {
    qRegisterMetaType<OpenGeoLab::App::LogEntry>("OpenGeoLab::App::LogEntry");
}

QAbstractItemModel* LogService::model() { return &m_model; }

bool LogService::hasNewErrors() const { return m_hasNewErrors; }

bool LogService::hasNewLogs() const { return m_hasNewLogs; }

void LogService::addEntry(LogEntry entry) {
    if(QThread::currentThread() == thread()) {
        addEntryOnUiThread(std::move(entry));
        return;
    }

    QPointer<LogService> self(this);
    QMetaObject::invokeMethod(
        this,
        [self, entry = std::move(entry)]() mutable {
            if(!self) {
                return;
            }
            self->addEntryOnUiThread(std::move(entry));
        },
        Qt::QueuedConnection);
}

void LogService::clear() {
    m_model.clear();

    const auto had_new_errors = m_hasNewErrors;
    const auto had_new_logs = m_hasNewLogs;
    m_hasNewErrors = false;
    m_hasNewLogs = false;

    if(had_new_errors) {
        emit hasNewErrorsChanged();
    }
    if(had_new_logs) {
        emit hasNewLogsChanged();
    }
}

void LogService::markAllSeen() {
    const auto had_new_errors = m_hasNewErrors;
    const auto had_new_logs = m_hasNewLogs;
    m_hasNewErrors = false;
    m_hasNewLogs = false;

    if(had_new_errors) {
        emit hasNewErrorsChanged();
    }
    if(had_new_logs) {
        emit hasNewLogsChanged();
    }
}

void LogService::addEntryOnUiThread(LogEntry entry) {
    m_model.append(std::move(entry));

    const auto had_new_logs = m_hasNewLogs;
    m_hasNewLogs = true;
    if(!had_new_logs) {
        emit hasNewLogsChanged();
    }

    if(entry.m_level >= 4) { // warn=3, err=4, critical=5
        const auto had_new_errors = m_hasNewErrors;
        m_hasNewErrors = true;
        if(!had_new_errors) {
            emit hasNewErrorsChanged();
        }
    }
}

} // namespace OpenGeoLab::App
