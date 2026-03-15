/**
 * @file OperationLogService.cpp
 * @brief Implementation of the QML-facing operation log service.
 */

#include "OperationLogService.hpp"

#include <ogl/core/ModuleLogger.hpp>

#include <QMetaObject>
#include <QPointer>
#include <QThread>

#include <algorithm>

namespace OGL::App {

OperationLogService::OperationLogService(QObject* parent)
    : QObject(parent), m_model(this), m_filterModel(this) {
    qRegisterMetaType<OperationLogEntry>("OGL::App::OperationLogEntry");
    m_filterModel.setSourceModel(&m_model);
}

QAbstractItemModel* OperationLogService::model() { return &m_filterModel; }

bool OperationLogService::hasNewErrors() const { return m_hasNewErrors; }

bool OperationLogService::hasNewLogs() const { return m_hasNewLogs; }

int OperationLogService::minLevel() const {
    return static_cast<int>(OGL::Core::currentModuleLoggerLevel());
}

int OperationLogService::enabledLevelMask() const {
    return m_filterModel.enabledLevelMask();
}

void OperationLogService::setMinLevel(int level) {
    const int clamped_level = std::clamp(level, 0, 6);
    if(minLevel() == clamped_level) {
        return;
    }

    OGL::Core::setModuleLoggerLevel(
        static_cast<spdlog::level::level_enum>(clamped_level));
    emit minLevelChanged();
}

bool OperationLogService::levelEnabled(int level) const {
    return m_filterModel.levelEnabled(level);
}

void OperationLogService::setLevelEnabled(int level, bool enabled) {
    if(m_filterModel.levelEnabled(level) == enabled) {
        return;
    }

    m_filterModel.setLevelEnabled(level, enabled);
    emit levelFilterChanged();
}

void OperationLogService::addEntry(OperationLogEntry entry) {
    if(QThread::currentThread() == thread()) {
        addEntryOnUiThread(std::move(entry));
        return;
    }

    QPointer<OperationLogService> self(this);
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

void OperationLogService::clear() {
    m_model.clear();
    markAllSeen();
}

void OperationLogService::markAllSeen() {
    const bool had_new_errors = m_hasNewErrors;
    const bool had_new_logs = m_hasNewLogs;

    m_hasNewErrors = false;
    m_hasNewLogs = false;

    if(had_new_errors) {
        emit hasNewErrorsChanged();
    }
    if(had_new_logs) {
        emit hasNewLogsChanged();
    }
}

void OperationLogService::addEntryOnUiThread(OperationLogEntry entry) {
    const bool is_error = entry.level >= 4;
    m_model.append(std::move(entry));

    const bool had_new_logs = m_hasNewLogs;
    m_hasNewLogs = true;
    if(!had_new_logs) {
        emit hasNewLogsChanged();
    }

    if(is_error) {
        const bool had_new_errors = m_hasNewErrors;
        m_hasNewErrors = true;
        if(!had_new_errors) {
            emit hasNewErrorsChanged();
        }
    }
}

} // namespace OGL::App
