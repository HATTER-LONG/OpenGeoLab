#include <util/qml_spdlog_sink.hpp>

#include <util/logger.hpp>

#include <QColor>
#include <QDateTime>

#include <spdlog/common.h>

#include <chrono>

namespace {

QColor levelToColor(int level) {
    // spdlog levels: trace=0, debug=1, info=2, warn=3, err=4, critical=5, off=6
    switch(level) {
    case 0:
        return QColor("#4e7aa6");
    case 1:
        return QColor("#4F83FF");
    case 2:
        return QColor("#E0E0E0");
    case 3:
        return QColor("#F4B400");
    case 4:
        return QColor("#EA4335");
    case 5:
        return QColor("#FF1744");
    default:
        return QColor("#E0E0E0");
    }
}

QString levelToName(int level) {
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

QDateTime toDateTime(const spdlog::log_clock::time_point& tp) {
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(ms));
}

} // namespace

namespace OpenGeoLab::Util {

QmlSpdlogSink::QmlSpdlogSink(OpenGeoLab::App::LogService* service) : m_service(service) {}

void QmlSpdlogSink::sink_it_(const spdlog::details::log_msg& msg) {
    if(!m_service) {
        return;
    }

    OpenGeoLab::App::LogEntry entry;
    entry.m_timestamp = toDateTime(msg.time);
    entry.m_level = static_cast<int>(msg.level);
    entry.m_levelName = levelToName(entry.m_level);
    entry.m_message = QString::fromUtf8(msg.payload.data(), static_cast<int>(msg.payload.size()));

    if(msg.source.filename != nullptr) {
        entry.m_file = QString::fromUtf8(msg.source.filename);
    }
    entry.m_line = msg.source.line;
    if(msg.source.funcname != nullptr) {
        entry.m_function = QString::fromUtf8(msg.source.funcname);
    }

    entry.m_levelColor = levelToColor(entry.m_level);

    m_service->addEntry(std::move(entry));
}

void installQmlSpdlogSink(OpenGeoLab::App::LogService* service) {
    if(service == nullptr) {
        return;
    }

    auto logger = OpenGeoLab::getLogger();
    if(!logger) {
        return;
    }

    auto sink = std::make_shared<QmlSpdlogSink>(service);
    sink->set_level(spdlog::level::trace);

    // Keep the sink lightweight: forward only the formatted payload; no extra formatting here.
    logger->sinks().push_back(std::move(sink));
}

} // namespace OpenGeoLab::Util
