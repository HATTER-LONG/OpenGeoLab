/**
 * @file qml_spdlog_sink.hpp
 * @brief spdlog sink that forwards log events to a QML-exposed LogService
 */

#pragma once

#include <app/log_service.hpp>

#include <QPointer>
#include <spdlog/details/log_msg.h>
#include <spdlog/sinks/base_sink.h>

#include <memory>
#include <mutex>

namespace OpenGeoLab::Util {

class QmlSpdlogSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit QmlSpdlogSink(OpenGeoLab::App::LogService* service);

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override {}

private:
    QPointer<OpenGeoLab::App::LogService> m_service;
};

/// Installs (appends) a QML sink to the global logger.
void installQmlSpdlogSink(OpenGeoLab::App::LogService* service);

} // namespace OpenGeoLab::Util
