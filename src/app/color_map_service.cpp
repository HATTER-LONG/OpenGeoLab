#include "app/color_map_service.hpp"

#include "util/color_map.hpp"

namespace OpenGeoLab::App {

ColorMapService::ColorMapService(QObject* parent) : QObject(parent) {}

ColorMapService::~ColorMapService() = default;

int ColorMapService::themeMode() const noexcept {
    return Util::ColorMap::instance().theme() == Util::ColorTheme::Light ? 0 : 1;
}

void ColorMapService::setThemeMode(int mode) {
    const int next_mode = (mode == 0) ? 0 : 1;
    auto& color_map = Util::ColorMap::mutableInstance();
    color_map.setThemeMode(next_mode);

    emit themeModeChanged(next_mode);
}

} // namespace OpenGeoLab::App
