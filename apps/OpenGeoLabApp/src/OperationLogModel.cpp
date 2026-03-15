/**
 * @file OperationLogModel.cpp
 * @brief Implementation of the internal operation activity model and filter.
 */

#include "OperationLogModel.hpp"

#include <QFileInfo>

namespace OGL::App {

OperationLogModel::OperationLogModel(QObject* parent) : QAbstractListModel(parent) {}

int OperationLogModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_entries.size());
}

QVariant OperationLogModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid()) {
        return {};
    }

    const auto row = index.row();
    if(row < 0 || row >= static_cast<int>(m_entries.size())) {
        return {};
    }

    const auto& entry = m_entries.at(static_cast<size_t>(row));
    switch(role) {
    case TimestampRole:
        return entry.timestamp;
    case TimeStringRole:
        return entry.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    case ThreadIdRole:
        return entry.threadId;
    case LevelRole:
        return entry.level;
    case LevelNameRole:
        return entry.levelName;
    case SourceRole:
        return entry.source;
    case MessageRole:
        return entry.message;
    case FileRole:
        return QFileInfo(entry.file).fileName();
    case LineRole:
        return entry.line;
    case FunctionNameRole:
        return entry.functionName;
    default:
        return {};
    }
}

QHash<int, QByteArray> OperationLogModel::roleNames() const {
    return {{TimestampRole, "timestamp"},
            {TimeStringRole, "time"},
            {ThreadIdRole, "threadId"},
            {LevelRole, "level"},
            {LevelNameRole, "levelName"},
            {SourceRole, "source"},
            {MessageRole, "message"},
            {FileRole, "file"},
            {LineRole, "line"},
            {FunctionNameRole, "functionName"}};
}

void OperationLogModel::append(OperationLogEntry entry) {
    const auto insert_row = static_cast<int>(m_entries.size());
    beginInsertRows(QModelIndex(), insert_row, insert_row);
    m_entries.push_back(std::move(entry));
    endInsertRows();

    trimToMaxEntries();
}

void OperationLogModel::clear() {
    if(m_entries.empty()) {
        return;
    }

    beginResetModel();
    m_entries.clear();
    endResetModel();
}

int OperationLogModel::maxEntries() const { return m_maxEntries; }

void OperationLogModel::setMaxEntries(int value) {
    m_maxEntries = value;
    trimToMaxEntries();
}

void OperationLogModel::trimToMaxEntries() {
    if(m_maxEntries <= 0 || static_cast<int>(m_entries.size()) <= m_maxEntries) {
        return;
    }

    const int remove_count = static_cast<int>(m_entries.size()) - m_maxEntries;
    beginRemoveRows(QModelIndex(), 0, remove_count - 1);
    for(int index = 0; index < remove_count; ++index) {
        m_entries.pop_front();
    }
    endRemoveRows();
}

OperationLogFilterModel::OperationLogFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
}

int OperationLogFilterModel::minLevel() const { return m_minLevel; }

void OperationLogFilterModel::setMinLevel(int level) {
    if(m_minLevel == level) {
        return;
    }

    m_minLevel = level;
    invalidateFilter();
    emit minLevelChanged();
}

int OperationLogFilterModel::enabledLevelMask() const {
    return static_cast<int>(m_enabledMask);
}

bool OperationLogFilterModel::levelEnabled(int level) const {
    if(level < 0 || level > 31) {
        return false;
    }
    return ((m_enabledMask >> static_cast<quint32>(level)) & 0x1u) != 0u;
}

void OperationLogFilterModel::setLevelEnabled(int level, bool enabled) {
    if(level < 0 || level > 31) {
        return;
    }

    const quint32 bit = 1u << static_cast<quint32>(level);
    const quint32 next_mask = enabled ? (m_enabledMask | bit) : (m_enabledMask & ~bit);
    if(next_mask == m_enabledMask) {
        return;
    }

    m_enabledMask = next_mask;
    invalidateFilter();
    emit levelFilterChanged();
}

bool OperationLogFilterModel::filterAcceptsRow(int source_row,
                                               const QModelIndex& source_parent) const {
    if(!sourceModel()) {
        return true;
    }

    const auto index = sourceModel()->index(source_row, 0, source_parent);
    const auto level = index.data(OperationLogModel::LevelRole).toInt();
    if(level < m_minLevel || !levelEnabled(level)) {
        return false;
    }

    return true;
}

auto operationLogLevelName(int level) -> QString {
    switch(level) {
    case 0:
        return QStringLiteral("trace");
    case 1:
        return QStringLiteral("debug");
    case 2:
        return QStringLiteral("info");
    case 3:
        return QStringLiteral("warn");
    case 4:
        return QStringLiteral("error");
    case 5:
        return QStringLiteral("critical");
    default:
        return QStringLiteral("log");
    }
}

} // namespace OGL::App
