/**
 * @file log_model.cpp
 * @brief Implementation of internal log model classes
 */

#include <app/log_model.hpp>

#include <QFileInfo>

namespace OpenGeoLab::App {

// ============================================================================
// LogEntryModel Implementation
// ============================================================================

LogEntryModel::LogEntryModel(QObject* parent) : QAbstractListModel(parent) {}

int LogEntryModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant LogEntryModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid()) {
        return {};
    }

    const auto row = index.row();
    if(row < 0 || row >= m_entries.size()) {
        return {};
    }

    const auto& entry = m_entries.at(row);

    switch(role) {
    case TimestampRole:
        return entry.m_timestamp;
    case TimeStringRole:
        return entry.m_timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz");
    case ThreadIdRole:
        return entry.m_threadId;
    case LevelRole:
        return entry.m_level;
    case LevelNameRole:
        return entry.m_levelName;
    case MessageRole:
        return entry.m_message;
    case FileRole:
        return QFileInfo(entry.m_file).fileName();
    case LineRole:
        return entry.m_line;
    case FunctionRole:
        return entry.m_function;
    case LevelColorRole:
        return entry.m_levelColor;
    default:
        return {};
    }
}

QHash<int, QByteArray> LogEntryModel::roleNames() const {
    return {
        {TimestampRole, "timestamp"}, {TimeStringRole, "time"},
        {ThreadIdRole, "tid"},        {LevelRole, "level"},
        {LevelNameRole, "levelName"}, {MessageRole, "message"},
        {FileRole, "file"},           {LineRole, "line"},
        {FunctionRole, "function"},   {LevelColorRole, "levelColor"},
    };
}

void LogEntryModel::append(LogEntry entry) {
    const auto insert_row = m_entries.size();

    beginInsertRows(QModelIndex(), insert_row, insert_row);
    m_entries.push_back(std::move(entry));
    endInsertRows();

    if(m_entries.size() > m_maxEntries && m_maxEntries > 0) {
        const int remove_count = m_entries.size() - m_maxEntries;
        beginRemoveRows(QModelIndex(), 0, remove_count - 1);
        m_entries.erase(m_entries.begin(), m_entries.begin() + remove_count);
        endRemoveRows();
    }
}

void LogEntryModel::clear() {
    if(m_entries.isEmpty()) {
        return;
    }
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

int LogEntryModel::maxEntries() const { return m_maxEntries; }

void LogEntryModel::setMaxEntries(int value) {
    m_maxEntries = value;
    if(m_entries.size() > m_maxEntries && m_maxEntries > 0) {
        const int remove_count = m_entries.size() - m_maxEntries;
        beginRemoveRows(QModelIndex(), 0, remove_count - 1);
        m_entries.erase(m_entries.begin(), m_entries.begin() + remove_count);
        endRemoveRows();
    }
}

// ============================================================================
// LogEntryFilterModel Implementation
// ============================================================================

LogEntryFilterModel::LogEntryFilterModel(QObject* parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
}

int LogEntryFilterModel::minLevel() const { return m_minLevel; }

void LogEntryFilterModel::setMinLevel(int level) {
    if(m_minLevel == level) {
        return;
    }
    m_minLevel = level;
    invalidateFilter();
    emit minLevelChanged();
}

bool LogEntryFilterModel::levelEnabled(int level) const {
    if(level < 0 || level > 31) {
        return false;
    }
    return ((m_enabledMask >> static_cast<quint32>(level)) & 0x1u) != 0u;
}

void LogEntryFilterModel::setLevelEnabled(int level, bool enabled) {
    if(level < 0 || level > 31) {
        return;
    }
    const quint32 bit = 1u << static_cast<quint32>(level);
    const quint32 mask = enabled ? (m_enabledMask | bit) : (m_enabledMask & ~bit);
    if(mask == m_enabledMask) {
        return;
    }
    m_enabledMask = mask;
    invalidateFilter();
    emit levelFilterChanged();
}

bool LogEntryFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if(!sourceModel()) {
        return true;
    }

    const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    const auto level = index.data(LogEntryModel::LevelRole).toInt();
    if(!levelEnabled(level)) {
        return false;
    }

    return true;
}

} // namespace OpenGeoLab::App
