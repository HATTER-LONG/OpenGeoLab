#include <app/log_entry_model.hpp>

#include <QFileInfo>

namespace OpenGeoLab::App {

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
        {TimestampRole, "timestamp"}, {TimeStringRole, "time"},   {LevelRole, "level"},
        {LevelNameRole, "levelName"}, {MessageRole, "message"},   {FileRole, "file"},
        {LineRole, "line"},           {FunctionRole, "function"}, {LevelColorRole, "levelColor"},
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

} // namespace OpenGeoLab::App
