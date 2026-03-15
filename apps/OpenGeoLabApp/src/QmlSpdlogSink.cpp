/**
 * @file QmlSpdlogSink.cpp
 * @brief Implementation of the QML-facing spdlog sink.
 */

#include "QmlSpdlogSink.hpp"

#include <QDateTime>

#include <chrono>

namespace {

auto toDateTime(const spdlog::log_clock::time_point& time_point) -> QDateTime {
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch())
            .count();
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(milliseconds));
}

} // namespace

namespace OGL::App {

QmlSpdlogSink::QmlSpdlogSink(OperationLogService* service) : m_service(service) {}

void QmlSpdlogSink::sink_it_(const spdlog::details::log_msg& msg) {
    if(!m_service) {
        return;
    }

    OperationLogEntry entry;
    entry.timestamp = toDateTime(msg.time);
    entry.level = static_cast<int>(msg.level);
    entry.levelName = operationLogLevelName(entry.level);
    entry.source = QString::fromUtf8(msg.logger_name.data(), static_cast<int>(msg.logger_name.size()));
    entry.message = QString::fromUtf8(msg.payload.data(), static_cast<int>(msg.payload.size()));
    entry.threadId = static_cast<qint64>(msg.thread_id);

    if(msg.source.filename != nullptr) {
        entry.file = QString::fromUtf8(msg.source.filename);
    }
    entry.line = msg.source.line;
    if(msg.source.funcname != nullptr) {
        entry.functionName = QString::fromUtf8(msg.source.funcname);
    }

    m_service->addEntry(std::move(entry));
}

auto createQmlSpdlogSink(OperationLogService* service) -> spdlog::sink_ptr {
    auto sink = std::make_shared<QmlSpdlogSink>(service);
    sink->set_level(spdlog::level::trace);
    return sink;
}

} // namespace OGL::App
