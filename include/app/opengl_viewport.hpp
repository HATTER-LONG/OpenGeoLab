
#pragma once

#include "render/render_scene_controller.hpp"
#include "render/trackball_controller.hpp"
#include "util/signal.hpp"

#include <QMatrix4x4>
#include <QQuickFramebufferObject>
#include <QSizeF>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit GLViewport(QQuickItem* parent = nullptr);
    ~GLViewport() override;

private slots:
    void onSceneNeedsUpdate();
    // void onRenderNeedsUpdate();

private:
    Render::CameraState m_cameraState;                 ///< Local camera state copy
    Render::TrackballController m_trackballController; ///< Camera manipulation controller

    Util::ScopedConnection m_sceneNeedsUpdateConn;
    Util::ScopedConnection m_cameraChangedConn;
    Util::ScopedConnection m_geometryChangedConn;
    Util::ScopedConnection m_meshChangedConn;
};

} // namespace OpenGeoLab::App
