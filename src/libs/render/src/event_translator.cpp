/**
 * @file event_translator.cpp
 * @brief EventTranslator implementation with Y-axis flip.
 */

#include <opengeolab/render/event_translator.hpp>

#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>

namespace OpenGeoLab::Render {

auto EventTranslator::translate_mouse_button(
    int button, bool pressed,
    float x, float y,
    int viewport_width, int viewport_height) -> std::unique_ptr<SoMouseButtonEvent> {

    auto event = std::make_unique<SoMouseButtonEvent>();

    // Y-axis flip: QML top-left origin → Coin3D bottom-left origin
    const auto coin_x = static_cast<short>(x);
    const auto coin_y = static_cast<short>(static_cast<float>(viewport_height) - y);
    event->setPosition(SbVec2s(coin_x, coin_y));

    // Map Qt button codes to Coin3D button enum
    SoMouseButtonEvent::Button coin_button = SoMouseButtonEvent::ANY;
    if (button == 1) {
        coin_button = SoMouseButtonEvent::BUTTON1;  // left
    } else if (button == 2) {
        coin_button = SoMouseButtonEvent::BUTTON2;  // right
    } else if (button == 4) {
        coin_button = SoMouseButtonEvent::BUTTON3;  // middle
    }
    event->setButton(coin_button);

    event->setState(pressed ? SoButtonEvent::DOWN : SoButtonEvent::UP);

    return event;
}

auto EventTranslator::translate_mouse_move(
    float x, float y,
    int viewport_width, int viewport_height) -> std::unique_ptr<SoLocation2Event> {

    auto event = std::make_unique<SoLocation2Event>();

    const auto coin_x = static_cast<short>(x);
    const auto coin_y = static_cast<short>(static_cast<float>(viewport_height) - y);
    event->setPosition(SbVec2s(coin_x, coin_y));

    return event;
}

} // namespace OpenGeoLab::Render
