/**
 * @file log_model.hpp
 * @brief Internal log model implementation details
 *
 * This header contains the internal implementation of log entry storage
 * and filtering. Use LogService for the public QML interface.
 */

#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QString>
#include <QtGlobal>

#include <deque>

namespace OpenGeoLab::App {

/**
 * @brief Data structure representing a single log entry
 */
struct LogEntry {
    QDateTime m_timestamp;
    int m_level{0};
    QString m_levelName;
    QString m_message;

    qint64 m_threadId{0};

    QString m_file;
    int m_line{0};
    QString m_function;

    QColor m_levelColor;
};

/**
 * @brief Model for storing and exposing log entries
 * @internal This class is used internally by LogService
 */
class LogEntryModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        TimestampRole = Qt::UserRole + 1,
        TimeStringRole,
        ThreadIdRole,
        LevelRole,
        LevelNameRole,
        MessageRole,
        FileRole,
        LineRole,
        FunctionRole,
        LevelColorRole,
    };
    Q_ENUM(Role)

    explicit LogEntryModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
    [[nodiscard]] int rowCount() const { return rowCount(QModelIndex()); }
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Append a log entry to the model
     * @note If maxEntries is positive, the oldest entries are trimmed after insertion.
     */
    void append(LogEntry entry);

    /**
     * @brief Remove all log entries
     */
    void clear();

    [[nodiscard]] int maxEntries() const;

    /**
     * @brief Set the maximum number of retained entries
     * @param value Maximum number of entries to keep; values <= 0 disable trimming
     */
    void setMaxEntries(int value);

private:
    void trimToMaxEntries();

    std::deque<LogEntry> m_entries;
    int m_maxEntries{2000};
};

/**
 * @brief Proxy model for filtering log entries by level
 *
 * Provides level-based filtering with individual level enable/disable support.
 * @internal This class is used internally by LogService
 */
class LogEntryFilterModel final : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int minLevel READ minLevel WRITE setMinLevel NOTIFY minLevelChanged)
public:
    explicit LogEntryFilterModel(QObject* parent = nullptr);

    /**
     * @brief Get the minimum log level for filtering
     * @return Minimum level value
     */
    [[nodiscard]] int minLevel() const;

    /**
     * @brief Set the minimum log level for filtering
     * @param level Minimum level value (entries below this are filtered out)
     */
    void setMinLevel(int level);

    /**
     * @brief Check if a specific log level is enabled
     * @param level Log level to check
     * @return true if the level is enabled for display
     */
    [[nodiscard]] bool levelEnabled(int level) const;

    /**
     * @brief Enable or disable a specific log level
     * @param level Log level to modify
     * @param enabled true to enable, false to disable
     */
    void setLevelEnabled(int level, bool enabled);

signals:
    void minLevelChanged();
    void levelFilterChanged();

protected:
    [[nodiscard]] bool filterAcceptsRow(int source_row,
                                        const QModelIndex& source_parent) const override;

private:
    int m_minLevel{0};
    quint32 m_enabledMask{0x3F};
};

} // namespace OpenGeoLab::App

Q_DECLARE_METATYPE(OpenGeoLab::App::LogEntry)
