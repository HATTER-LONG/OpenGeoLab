/**
 * @file geometry_creator.hpp
 * @brief Factory for creating basic 3D geometry using OCC
 *
 * Provides methods to create common geometric primitives (box, cylinder, sphere, etc.)
 * using Open CASCADE Technology. The shapes are automatically triangulated for rendering.
 */

#pragma once

#include <geometry/geometry.hpp>

#include <QObject>
#include <QQmlEngine>
#include <memory>

#include <TopoDS_Shape.hxx>

namespace OpenGeoLab {
namespace UI {
class Geometry3D;

/**
 * @brief QML singleton for creating geometric primitives using OCC
 *
 * This class provides QML bindings for creating basic 3D shapes.
 * All shapes are created using Open CASCADE and automatically
 * triangulated for rendering.
 */
class GeometryCreator : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit GeometryCreator(QObject* parent = nullptr);

    /**
     * @brief Set the target renderer for created geometry
     * @param renderer QObject pointer to Geometry3D instance
     */
    Q_INVOKABLE void setTargetRenderer(QObject* renderer);

    /**
     * @brief Create a box with given dimensions
     * @param width Width (X dimension)
     * @param height Height (Y dimension)
     * @param depth Depth (Z dimension)
     * @return true on success, false on failure
     */
    Q_INVOKABLE bool createBox(qreal width, qreal height, qreal depth);

    /**
     * @brief Create a cylinder
     * @param radius Cylinder radius
     * @param height Cylinder height
     * @return true on success, false on failure
     */
    Q_INVOKABLE bool createCylinder(qreal radius, qreal height);

    /**
     * @brief Create a sphere
     * @param radius Sphere radius
     * @return true on success, false on failure
     */
    Q_INVOKABLE bool createSphere(qreal radius);

    /**
     * @brief Create a cone
     * @param bottom_radius Bottom radius
     * @param top_radius Top radius (0 for pointed cone)
     * @param height Cone height
     * @return true on success, false on failure
     */
    Q_INVOKABLE bool createCone(qreal bottom_radius, qreal top_radius, qreal height);

    /**
     * @brief Create a torus
     * @param major_radius Distance from center to tube center
     * @param minor_radius Tube radius
     * @return true on success, false on failure
     */
    Q_INVOKABLE bool createTorus(qreal major_radius, qreal minor_radius);

signals:
    /**
     * @brief Emitted when geometry is successfully created
     * @param name Name/description of the created geometry
     */
    void geometryCreated(const QString& name);

    /**
     * @brief Emitted when geometry creation fails
     * @param error Error message describing the failure
     */
    void geometryCreationFailed(const QString& error);

private:
    /**
     * @brief Create geometry from OCC shape and send to renderer
     * @param shape The OCC shape to triangulate and display
     * @param name Name for logging and signals
     * @return true on success, false on failure
     */
    bool createFromShape(const TopoDS_Shape& shape, const QString& name);

    Geometry3D* m_targetRenderer = nullptr;
};

} // namespace UI
} // namespace OpenGeoLab
