// cube_renderer.h - Cube-specific 3D renderer
// Specialized renderer for rendering cube geometry
#pragma once

#include "geometry.h"
#include "opengl_renderer.h"
#include <QtQuick/QQuickItem>
#include <memory>

/**
 * @brief Renderer specifically for cube geometry
 *
 * This class extends OpenGL3DRenderer to provide cube-specific
 * rendering functionality. It handles cube geometry initialization
 * and can be easily extended for other geometric shapes.
 */
class CubeRenderer : public OpenGL3DRenderer {
    Q_OBJECT

public:
    CubeRenderer();
    ~CubeRenderer() override = default;

    void init() override;

private:
    void initializeCubeGeometry();
};

/**
 * @brief QML item for 3D cube rendering
 *
 * This QQuickItem provides a cube rendering component that can be
 * used in QML. It manages the lifecycle of the CubeRenderer and
 * connects it to the Qt Quick scene graph.
 */
class Cube3D : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

public:
    Cube3D();
    ~Cube3D() override = default;

signals:
    void rendererReady();

public slots:
    /**
     * @brief Synchronize QML item state with renderer
     * Called before rendering to update renderer parameters
     */
    void sync();

    /**
     * @brief Clean up OpenGL resources
     */
    void cleanup();

private slots:
    void handleWindowChanged(QQuickWindow* win);

private:
    void releaseResources() override;

    CubeRenderer* m_renderer = nullptr;
};
