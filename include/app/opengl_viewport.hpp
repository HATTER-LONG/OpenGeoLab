
#pragma once

#include "util/signal.hpp"

#include <QMatrix4x4>
#include <QQuickFramebufferObject>
#include <QSizeF>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hasGeometry READ hasGeometry NOTIFY hasGeometryChanged)

public:
    explicit GLViewport(QQuickItem* parent = nullptr);
    ~GLViewport() override;
};

} // namespace OpenGeoLab::App
