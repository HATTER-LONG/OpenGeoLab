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

/**
 * @brief QML singleton service for application log management
 *
 * Provides a thread-safe interface for adding log entries and exposes
 * them to QML through a filtered model. Supports level-based filtering
 * and tracks new/unread log status.
 */
class LogService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)
    Q_PROPERTY(bool hasNewErrors READ hasNewErrors NOTIFY hasNewErrorsChanged)
    Q_PROPERTY(bool hasNewLogs READ hasNewLogs NOTIFY hasNewLogsChanged)
    Q_PROPERTY(int minLevel READ minLevel WRITE setMinLevel NOTIFY minLevelChanged)
public:
    explicit LogService(QObject* parent = nullptr);

    /**
     * @brief Get the filtered log model for QML binding
     * @return Pointer to the filtered item model
     */
    [[nodiscard]] QAbstractItemModel* model();

    [[nodiscard]] bool hasNewErrors() const;
    [[nodiscard]] bool hasNewLogs() const;
    [[nodiscard]] int minLevel() const;

    /**
     * @brief Set the minimum log level for filtering
     * @param level Minimum level value
     */
    Q_INVOKABLE void setMinLevel(int level);

    /**
     * @brief Check if a specific log level is enabled
     * @param level Log level to check
     * @return true if the level is enabled
     */
    Q_INVOKABLE bool levelEnabled(int level) const;

    /**
     * @brief Enable or disable a specific log level
     * @param level Log level to modify
     * @param enabled true to enable, false to disable
     */
    Q_INVOKABLE void setLevelEnabled(int level, bool enabled);

    /**
     * @brief Add a log entry to the model
     * @param entry Log entry to add
     * @note Thread-safe: can be called from any thread
     */
    void addEntry(LogEntry entry);

    /**
     * @brief Clear all log entries
     */
    Q_INVOKABLE void clear();

    /**
     * @brief Mark all logs as seen, resetting new log/error flags
     */
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
