/**
 * @file request_recorder.cpp
 * @brief Implements request recording for serialized protocol requests.
 */

#include <opengeolab/command/request_recorder.hpp>

namespace OpenGeoLab::Command {

void RequestRecorder::start() { m_recording = true; }

void RequestRecorder::stop() { m_recording = false; }

void RequestRecorder::record(std::string_view request_json) {
    if(!m_recording) {
        return;
    }

    m_requests.emplace_back(request_json);
}

auto RequestRecorder::get() const -> const std::vector<std::string>& { return m_requests; }

auto RequestRecorder::isRecording() const noexcept -> bool { return m_recording; }

void RequestRecorder::clear() { m_requests.clear(); }

} // namespace OpenGeoLab::Command
