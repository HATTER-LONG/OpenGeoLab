/**
 * @file log_event_model.cpp
 * @brief Implementation of LogEventModel and QmlLogSink.
 */

#include "opengeolab/app/log_event_model.h"

#include <QMetaObject>
#include <QTime>

#include <chrono>
#include <ctime>
#include <string_view>

namespace OpenGeoLab::App {

// ---------------------------------------------------------------------------
// LogEventModel
// ---------------------------------------------------------------------------

LogEventModel::LogEventModel(QObject* parent) : QAbstractListModel(parent) {}

int LogEventModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_entries.size());
}

QVariant LogEventModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_entries.size())) {
        return {};
    }

    const auto& entry = m_entries[static_cast<size_t>(index.row())];
    switch(role) {
    case LevelRole:
        return entry.level;
    case LevelNameRole:
        return entry.levelName;
    case SourceRole:
        return entry.source;
    case MessageRole:
        return entry.message;
    case TimeRole:
        return entry.time;
    case ThreadIdRole:
        return entry.threadId;
    case FileRole:
        return entry.file;
    case LineRole:
        return entry.line;
    default:
        return {};
    }
}

QHash<int, QByteArray> LogEventModel::roleNames() const {
    return {
        {LevelRole, "level"},     {LevelNameRole, "levelName"}, {SourceRole, "source"},
        {MessageRole, "message"}, {TimeRole, "time"},           {ThreadIdRole, "threadId"},
        {FileRole, "file"},       {LineRole, "line"},
    };
}

void LogEventModel::clear() {
    if(m_entries.empty()) {
        return;
    }
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

int LogEventModel::runtimeMinLevel() const { return m_runtimeMinLevel; }

void LogEventModel::setRuntimeMinLevel(int level) {
    level = std::clamp(level, 0, 5);
    if(m_runtimeMinLevel == level) {
        return;
    }
    m_runtimeMinLevel = level;

    if(m_logger) {
        m_logger->set_level(static_cast<spdlog::level::level_enum>(level));
    }

    emit runtimeMinLevelChanged();
}

int LogEventModel::count() const { return static_cast<int>(m_entries.size()); }

void LogEventModel::installSink(const std::shared_ptr<spdlog::logger>& logger) {
    m_logger = logger;

    auto sink = std::make_shared<QmlLogSink>(this);
    logger->sinks().push_back(sink);
}

void LogEventModel::appendEntry(int level,
                                const QString& levelName,
                                const QString& source,
                                const QString& message,
                                const QString& time,
                                int threadId,
                                const QString& file,
                                int line) {
    // Trim old entries if exceeding capacity
    if(static_cast<int>(m_entries.size()) >= MAX_ENTRIES) {
        const int removeCount = MAX_ENTRIES / 4;
        beginRemoveRows(QModelIndex(), 0, removeCount - 1);
        m_entries.erase(m_entries.begin(), m_entries.begin() + removeCount);
        endRemoveRows();
    }

    const int row = static_cast<int>(m_entries.size());
    beginInsertRows(QModelIndex(), row, row);
    m_entries.push_back({level, levelName, source, message, time, threadId, file, line});
    endInsertRows();

    emit countChanged();
    emit newEntryAdded(level);
}

// ---------------------------------------------------------------------------
// QmlLogSink
// ---------------------------------------------------------------------------

QmlLogSink::QmlLogSink(LogEventModel* model) : m_model(model) {}

void QmlLogSink::sink_it_(const spdlog::details::log_msg& msg) {
    const int level = static_cast<int>(msg.level);

    // Level name mapping (spdlog: err → display: ERROR)
    static constexpr const char* LEVEL_NAMES[] = {"TRACE", "DEBUG", "INFO",
                                                  "WARN",  "ERROR", "CRITICAL"};
    const QString levelName = (level >= 0 && level <= 5) ? QString::fromLatin1(LEVEL_NAMES[level])
                                                         : QStringLiteral("UNKNOWN");

    // Extract source component name from file path
    QString source = QStringLiteral("OpenGeoLab");
    QString file;
    int line = 0;

    if(!msg.source.empty()) {
        const std::string_view fullPath(msg.source.filename);
        const auto lastSep = fullPath.find_last_of("/\\");
        const auto filename =
            (lastSep != std::string_view::npos) ? fullPath.substr(lastSep + 1) : fullPath;
        const auto dot = filename.find_last_of('.');
        const auto stem = (dot != std::string_view::npos) ? filename.substr(0, dot) : filename;

        source = QString::fromUtf8(stem.data(), static_cast<qsizetype>(stem.size()));
        file = QString::fromUtf8(filename.data(), static_cast<qsizetype>(filename.size()));
        line = msg.source.line;
    }

    // Message payload
    const QString message =
        QString::fromUtf8(msg.payload.data(), static_cast<qsizetype>(msg.payload.size()));

    // Timestamp from the log message
    const auto epoch = msg.time.time_since_epoch();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
    const std::time_t timeVal = seconds.count();
    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &timeVal);
#else
    localtime_r(&timeVal, &tmBuf);
#endif
    const QString timeStr =
        QString::asprintf("%02d:%02d:%02d", tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec);

    const int threadId = static_cast<int>(msg.thread_id);

    // Post to the main thread
    QMetaObject::invokeMethod(m_model, "appendEntry", Qt::QueuedConnection, Q_ARG(int, level),
                              Q_ARG(QString, levelName), Q_ARG(QString, source),
                              Q_ARG(QString, message), Q_ARG(QString, timeStr),
                              Q_ARG(int, threadId), Q_ARG(QString, file), Q_ARG(int, line));
}

void QmlLogSink::flush_() {
    // No buffering; entries are posted immediately via QueuedConnection.
}

} // namespace OpenGeoLab::App
