/**
 * @file log_service.hpp
 * @brief QObject service for exposing application logs to QML
 *
 * This is the public interface for the logging system.
 * Use this class to interact with logs from QML.
 */

#pragma once

#include <app/log_model.hpp>

#include <QAbstractItemModel>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

class LogService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)
    Q_PROPERTY(bool hasNewErrors READ hasNewErrors NOTIFY hasNewErrorsChanged)
    Q_PROPERTY(bool hasNewLogs READ hasNewLogs NOTIFY hasNewLogsChanged)
    Q_PROPERTY(int minLevel READ minLevel WRITE setMinLevel NOTIFY minLevelChanged)
public:
    explicit LogService(QObject* parent = nullptr);

    [[nodiscard]] QAbstractItemModel* model();

    [[nodiscard]] bool hasNewErrors() const;
    [[nodiscard]] bool hasNewLogs() const;
    [[nodiscard]] int minLevel() const;
    Q_INVOKABLE void setMinLevel(int level);

    Q_INVOKABLE bool levelEnabled(int level) const;
    Q_INVOKABLE void setLevelEnabled(int level, bool enabled);

    /// Thread-safe: can be called from any thread.
    void addEntry(LogEntry entry);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void markAllSeen();

signals:
    void hasNewErrorsChanged();
    void hasNewLogsChanged();
    void minLevelChanged();
    void levelFilterChanged();

private:
    void addEntryOnUiThread(LogEntry entry);

private:
    LogEntryModel m_model;
    LogEntryFilterModel m_filterModel;
    bool m_hasNewErrors{false};
    bool m_hasNewLogs{false};
};

} // namespace OpenGeoLab::App
