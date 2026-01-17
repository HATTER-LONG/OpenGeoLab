#include <app/log_service.hpp>

#include <QMetaObject>
#include <QThread>

namespace OpenGeoLab::App {

LogService::LogService(QObject* parent) : QObject(parent), m_model(this), m_filterModel(this) {
    qRegisterMetaType<LogEntry>("OpenGeoLab::App::LogEntry");
    m_filterModel.setSourceModel(&m_model);
}

QAbstractItemModel* LogService::model() { return &m_filterModel; }

bool LogService::hasNewErrors() const { return m_hasNewErrors; }

bool LogService::hasNewLogs() const { return m_hasNewLogs; }

int LogService::minLevel() const { return m_filterModel.minLevel(); }

void LogService::setMinLevel(int level) {
    if(m_filterModel.minLevel() == level) {
        return;
    }
    m_filterModel.setMinLevel(level);
    emit minLevelChanged();
}

bool LogService::levelEnabled(int level) const { return m_filterModel.levelEnabled(level); }

void LogService::setLevelEnabled(int level, bool enabled) {
    if(m_filterModel.levelEnabled(level) == enabled) {
        return;
    }
    m_filterModel.setLevelEnabled(level, enabled);
    emit levelFilterChanged();
}

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
