/**
 * @file event_translator.hpp
 * @brief Translates abstract input events into Coin3D SoEvent objects.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

#include <memory>

class SoMouseButtonEvent;
class SoLocation2Event;

namespace OpenGeoLab::Render {

/**
 * @brief Converts QML mouse events into Coin3D SoEvent objects.
 *
 * Handles coordinate system conversion: QML origin is top-left,
 * Coin3D origin is bottom-left (Y-axis flip).
 */
class OPENGEOLAB_RENDER_EXPORT EventTranslator {
public:
    /**
     * @brief Translate a mouse button press/release into SoMouseButtonEvent.
     * @param button Qt mouse button code (1=left, 2=right, 4=middle)
     * @param pressed true for press, false for release
     * @param x QML x coordinate (origin top-left)
     * @param y QML y coordinate (origin top-left)
     * @param viewport_width Viewport width in pixels
     * @param viewport_height Viewport height in pixels
     */
    static auto translate_mouse_button(
        int button, bool pressed,
        float x, float y,
        int viewport_width, int viewport_height) -> std::unique_ptr<SoMouseButtonEvent>;

    /**
     * @brief Translate a mouse move into SoLocation2Event.
     * @param x QML x coordinate (origin top-left)
     * @param y QML y coordinate (origin top-left)
     * @param viewport_width Viewport width in pixels
     * @param viewport_height Viewport height in pixels
     */
    static auto translate_mouse_move(
        float x, float y,
        int viewport_width, int viewport_height) -> std::unique_ptr<SoLocation2Event>;
};

} // namespace OpenGeoLab::Render
