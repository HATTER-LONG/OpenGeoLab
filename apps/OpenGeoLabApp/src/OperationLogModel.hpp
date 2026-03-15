/**
 * @file OperationLogModel.hpp
 * @brief Internal model and filter types for UI-visible application logs.
 */

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QString>
#include <QtGlobal>

#include <deque>

namespace OGL::App {

struct OperationLogEntry {
    QDateTime timestamp;
    int level{0};
    QString levelName;
    QString source;
    QString message;
    qint64 threadId{0};
    QString file;
    int line{0};
    QString functionName;
};

class OperationLogModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        TimeStringRole,
        ThreadIdRole,
        LevelRole,
        LevelNameRole,
        SourceRole,
        MessageRole,
        FileRole,
        LineRole,
        FunctionNameRole
    };
    Q_ENUM(Roles)

    explicit OperationLogModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void append(OperationLogEntry entry);
    void clear();

    [[nodiscard]] int maxEntries() const;
    void setMaxEntries(int value);

private:
    void trimToMaxEntries();

    std::deque<OperationLogEntry> m_entries;
    int m_maxEntries = 2000;
};

class OperationLogFilterModel final : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int minLevel READ minLevel WRITE setMinLevel NOTIFY minLevelChanged)
    Q_PROPERTY(int enabledLevelMask READ enabledLevelMask NOTIFY levelFilterChanged)

public:
    explicit OperationLogFilterModel(QObject* parent = nullptr);

    [[nodiscard]] int minLevel() const;
    void setMinLevel(int level);

    [[nodiscard]] int enabledLevelMask() const;
    [[nodiscard]] bool levelEnabled(int level) const;
    void setLevelEnabled(int level, bool enabled);

signals:
    void minLevelChanged();
    void levelFilterChanged();

protected:
    [[nodiscard]] bool filterAcceptsRow(int source_row,
                                        const QModelIndex& source_parent) const override;

private:
    int m_minLevel = 0;
    quint32 m_enabledMask = 0x3F;
};

[[nodiscard]] auto operationLogLevelName(int level) -> QString;

} // namespace OGL::App

Q_DECLARE_METATYPE(OGL::App::OperationLogEntry)
