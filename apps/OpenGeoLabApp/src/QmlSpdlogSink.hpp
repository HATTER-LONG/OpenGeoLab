/**
 * @file QmlSpdlogSink.hpp
 * @brief spdlog sink that forwards log events into OperationLogService.
 */

#pragma once

#include "OperationLogService.hpp"

#include <QPointer>
#include <spdlog/details/log_msg.h>
#include <spdlog/sinks/base_sink.h>

#include <mutex>

namespace OGL::App {

class QmlSpdlogSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit QmlSpdlogSink(OperationLogService* service);

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override {}

private:
    QPointer<OperationLogService> m_service;
};

[[nodiscard]] auto createQmlSpdlogSink(OperationLogService* service) -> spdlog::sink_ptr;

} // namespace OGL::App
