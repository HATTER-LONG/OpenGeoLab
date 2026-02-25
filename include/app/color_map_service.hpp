/**
 * @file color_map_service.hpp
 * @brief QML singleton bridge for synchronizing UI theme and render ColorMap
 */

#pragma once

#include <QObject>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

class ColorMapService final : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int themeMode READ themeMode NOTIFY themeModeChanged)

public:
    explicit ColorMapService(QObject* parent = nullptr);
    ~ColorMapService() override;

    [[nodiscard]] int themeMode() const noexcept;

    Q_INVOKABLE void setThemeMode(int mode);

signals:
    void themeModeChanged(int mode);
};

} // namespace OpenGeoLab::App
