/**
 * @file log_event_model.h
 * @brief QAbstractListModel bridging spdlog to QML, plus a custom spdlog sink.
 *
 * LogEventModel stores log entries and exposes them as a QML list model.
 * QmlLogSink is a spdlog sink that captures log events from any thread
 * and posts them to LogEventModel on the main thread via QueuedConnection.
 */

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

#include <mutex>
#include <vector>

namespace OpenGeoLab::App {

/**
 * @brief QML list model storing spdlog log entries.
 *
 * Registered as a QML singleton ("LogEventModel") in OpenGeoLab.Services.
 * Provides:
 * - Role-based access for QML ListView delegates
 * - runtimeMinLevel property controlling the spdlog logger level
 * - clear() to empty the log buffer
 */
class LogEventModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int runtimeMinLevel READ runtimeMinLevel WRITE setRuntimeMinLevel NOTIFY
                   runtimeMinLevelChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    /** @brief Custom roles exposed to QML delegates. */
    enum Roles {
        LevelRole = Qt::UserRole + 1,
        LevelNameRole,
        SourceRole,
        MessageRole,
        TimeRole,
        ThreadIdRole,
        FileRole,
        LineRole
    };

    explicit LogEventModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();

    [[nodiscard]] int runtimeMinLevel() const;
    void setRuntimeMinLevel(int level);
    [[nodiscard]] int count() const;

    /**
     * @brief Install the QML log sink into the given spdlog logger.
     *
     * After calling this, all log events dispatched through the logger will
     * appear in this model (subject to the logger level).
     */
    void installSink(const std::shared_ptr<spdlog::logger>& logger);

Q_SIGNALS:
    void runtimeMinLevelChanged();
    void countChanged();

    /** @brief Emitted when a new entry is added; level is 0-5. */
    void newEntryAdded(int level);

public Q_SLOTS:
    /** @brief Append a log entry (called from QmlLogSink via QueuedConnection). */
    void appendEntry(int level,
                     const QString& level_name,
                     const QString& source,
                     const QString& message,
                     const QString& time,
                     int thread_id,
                     const QString& file,
                     int line);

private:
    struct LogEntry {
        int level;
        QString levelName;
        QString source;
        QString message;
        QString time;
        int threadId;
        QString file;
        int line;
    };

    std::vector<LogEntry> m_entries;
    int m_runtimeMinLevel{2}; ///< Default: INFO
    std::shared_ptr<spdlog::logger> m_logger;
    static constexpr int MAX_ENTRIES = 2000;
};

/**
 * @brief Custom spdlog sink forwarding log events to LogEventModel.
 *
 * Thread-safe: uses base_sink<std::mutex> for synchronization.
 * Posts entries to the LogEventModel on the main thread via
 * QMetaObject::invokeMethod with Qt::QueuedConnection.
 */
class QmlLogSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit QmlLogSink(LogEventModel* model);

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override;

private:
    LogEventModel* m_model;
};

} // namespace OpenGeoLab::App
