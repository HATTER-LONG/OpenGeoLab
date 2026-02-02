/**
 * @file qml_spdlog_sink.hpp
 * @brief spdlog sink that forwards log events to a QML-exposed LogService
 *
 * Provides integration between spdlog logging framework and the QML-based
 * LogService, allowing log messages to be displayed in the application UI.
 */

#pragma once

#include <app/log_service.hpp>

#include <QPointer>
#include <spdlog/details/log_msg.h>
#include <spdlog/sinks/base_sink.h>

#include <mutex>

namespace OpenGeoLab::Util {

/**
 * @brief spdlog sink that forwards log messages to LogService
 *
 * This sink receives log messages from spdlog and forwards them to
 * the QML LogService for display in the application UI.
 */
class QmlSpdlogSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    /**
     * @brief Construct a QML spdlog sink
     * @param service LogService instance to forward messages to
     */
    explicit QmlSpdlogSink(OpenGeoLab::App::LogService* service);

protected:
    /**
     * @brief Process and forward a log message
     * @param msg Log message to process
     */
    void sink_it_(const spdlog::details::log_msg& msg) override;

    /**
     * @brief Flush pending messages (no-op for this sink)
     */
    void flush_() override {}

private:
    QPointer<OpenGeoLab::App::LogService> m_service; ///< Target LogService
};

/**
 * @brief Install a QML sink on the global logger
 * @param service LogService instance to receive forwarded messages
 */
void installQmlSpdlogSink(OpenGeoLab::App::LogService* service);

} // namespace OpenGeoLab::Util
