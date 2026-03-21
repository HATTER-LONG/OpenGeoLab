/**
 * @file request_recorder.hpp
 * @brief Declares a lightweight recorder for serialized protocol requests.
 */

#pragma once

#include <opengeolab/command/command_export.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Command {

/**
 * @brief Records serialized JSON requests while capture is enabled.
 *
 * The recorder preserves previously captured requests until clear() is called.
 * Calling start() after a prior recording session resumes appending to the
 * existing buffer.
 *
 * @note Thread safety: not internally synchronized. Current architecture
 * guarantees serial access via Python GIL. If GIL is released during
 * dispatch in the future, external synchronization must be added.
 */
class OPENGEOLAB_COMMAND_EXPORT RequestRecorder {
public:
    /**
     * @brief Enables request recording without clearing the existing buffer.
     */
    void start();

    /**
     * @brief Disables request recording.
     */
    void stop();

    /**
     * @brief Appends a serialized JSON request when recording is enabled.
     * @param request_json Serialized request envelope to store.
     */
    void record(std::string_view request_json);

    /**
     * @brief Returns the recorded requests in insertion order.
     * @return Immutable reference to the internal request buffer.
     */
    [[nodiscard]] auto get() const -> const std::vector<std::string>&;

    /**
     * @brief Reports whether recording is currently enabled.
     * @return True when record() appends requests; otherwise false.
     */
    [[nodiscard]] auto isRecording() const noexcept -> bool;

    /**
     * @brief Removes all recorded requests without changing recording state.
     */
    void clear();

private:
    bool m_recording = false;
    std::vector<std::string> m_requests;
};

} // namespace OpenGeoLab::Command
