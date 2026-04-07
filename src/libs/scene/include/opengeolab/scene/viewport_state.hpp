/**
 * @file viewport_state.hpp
 * @brief ViewportState — mutex-protected camera + pending pick area
 *
 * Single source of truth for camera state. Shared by GUI thread
 * (TrackballController), worker threads (actions), and render thread
 * (synchronize reads camera snapshot).
 */

#pragma once

#include <opengeolab/core/pick_action.hpp>
#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/camera_state.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <opengeolab/scene/view_preset.hpp>

#include <kangaroo/util/signal.hpp>

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace OpenGeoLab::Scene {

/// @brief Coordinate type for pick area requests.
enum class PickAreaCoordType { Normalized, Pixel };

/// @brief Pending pick area request (async, consumed by renderer).
struct PendingPickArea {
    float x0{0.0F}; ///< Left/start X
    float y0{0.0F}; ///< Top/start Y
    float x1{0.0F}; ///< Right/end X
    float y1{0.0F}; ///< Bottom/end Y
    PickAreaCoordType coordType{PickAreaCoordType::Normalized};
    Core::PickAction action{Core::PickAction::Add};
};

/// @brief Pending viewport capture request (async, consumed by renderer).
struct CaptureResult {
    std::string image;          ///< Base64 PNG for JSON response
    std::string imageError;     ///< Error while producing JSON image data
    std::string savedPath;      ///< Output path written successfully
    std::string savedPathError; ///< Error while writing outputPath
};

/// @brief Pending viewport capture request (async, consumed by renderer).
struct PendingCapture {
    int width{1024};             ///< Desired capture width in pixels
    int height{768};             ///< Desired capture height in pixels
    std::string outputPath;      ///< Optional PNG output path
    bool captureImage{true};     ///< Whether JSON image data is requested
    /// Shared promise to deliver capture results back to the requester.
    std::shared_ptr<std::promise<CaptureResult>> promise;
};

/**
 * @brief Thread-safe viewport state: camera + pending pick/capture requests.
 *
 * All accessors acquire m_mutex. Signals are emitted after releasing
 * the lock to avoid deadlock in signal handlers.
 */
class OPENGEOLAB_SCENE_EXPORT ViewportState final {
public:
    ViewportState();
    ~ViewportState();

    /// @brief Read current camera state (lock → copy → unlock).
    [[nodiscard]] CameraState camera() const;

    /// @brief Replace camera state (lock → set → unlock, bump version).
    void setCamera(const CameraState& state);

    /// @brief Monotonic camera version counter.
    [[nodiscard]] uint64_t cameraVersion() const noexcept;

    /// @brief Frame camera to view the given bounding box.
    void fitToBounds(const BoundingBox3D& bounds);

    /// @brief Apply a standard camera view preset.
    void setViewPreset(ViewPreset preset);

    /// @brief Queue an async pick area request for the renderer.
    void requestPickArea(const PendingPickArea& request);

    /// @brief Consume and return the pending pick area (if any).
    [[nodiscard]] std::optional<PendingPickArea> consumePickArea();

    /// @brief Queue a viewport capture request for the renderer.
    void requestCapture(PendingCapture request);

    /// @brief Consume and return the pending capture (if any).
    [[nodiscard]] std::optional<PendingCapture> consumeCapture();

    Kangaroo::Util::Signal<> cameraChanged;      ///< Fired after camera mutation
    Kangaroo::Util::Signal<> pickAreaRequested;  ///< Fired after pick area queued
    Kangaroo::Util::Signal<> captureRequested;   ///< Fired after capture queued

    /// @brief Whether x-ray rendering is enabled.
    [[nodiscard]] bool xRayMode() const;

    /// @brief Enable or disable x-ray rendering.
    void setXRayMode(bool enabled);

    /// @brief Whether tessellation overlay is enabled.
    [[nodiscard]] bool showTessellation() const;

    /// @brief Enable or disable tessellation mesh overlay.
    void setShowTessellation(bool enabled);

    Kangaroo::Util::Signal<> displayModeChanged; ///< Fired after xRay/tessellation change

private:
    mutable std::mutex m_mutex;
    CameraState m_camera;
    std::atomic<uint64_t> m_cameraVersion{0};
    std::optional<PendingPickArea> m_pendingPickArea;
    std::optional<PendingCapture> m_pendingCapture;
    bool m_xRayMode{false};
    bool m_showTessellation{false};
};

} // namespace OpenGeoLab::Scene
