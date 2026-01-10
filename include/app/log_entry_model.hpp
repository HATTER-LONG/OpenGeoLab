/**
 * @file log_entry_model.hpp
 * @brief QAbstractListModel for exposing spdlog logs to QML
 */

#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDateTime>
#include <QString>
#include <QVector>

namespace OpenGeoLab::App {

struct LogEntry {
    QDateTime m_timestamp;
    int m_level{0};
    QString m_levelName;
    QString m_message;

    QString m_file;
    int m_line{0};
    QString m_function;

    QColor m_levelColor;
};

class LogEntryModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        TimestampRole = Qt::UserRole + 1,
        TimeStringRole,
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

    void append(LogEntry entry);
    void clear();

    [[nodiscard]] int maxEntries() const;
    void setMaxEntries(int value);

private:
    QVector<LogEntry> m_entries;
    int m_maxEntries{2000};
};

} // namespace OpenGeoLab::App

Q_DECLARE_METATYPE(OpenGeoLab::App::LogEntry)
