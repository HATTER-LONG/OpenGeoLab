/**
 * @file geometry_creator.cpp
 * @brief Implementation of geometry creator using OCC primitives
 */

#include <geometry/shape_triangulator.hpp>
#include <ui/geometry3d.hpp>
#include <ui/geometry_creator.hpp>


#include <core/logger.hpp>

// Open CASCADE includes for primitive creation
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>


namespace OpenGeoLab {
namespace UI {

GeometryCreator::GeometryCreator(QObject* parent) : QObject(parent) {
    LOG_INFO("GeometryCreator initialized");
}

void GeometryCreator::setTargetRenderer(QObject* renderer) {
    m_targetRenderer = qobject_cast<Geometry3D*>(renderer);
    if(m_targetRenderer) {
        LOG_INFO("GeometryCreator target renderer set successfully");
    } else {
        LOG_DEBUG("GeometryCreator: Failed to set target renderer - invalid object type");
    }
}

bool GeometryCreator::createFromShape(const TopoDS_Shape& shape, const QString& name) {
    if(!m_targetRenderer) {
        QString error = "No target renderer set";
        LOG_ERROR(error.toStdString());
        emit geometryCreationFailed(error);
        return false;
    }

    if(shape.IsNull()) {
        QString error = "Shape is null";
        LOG_ERROR(error.toStdString());
        emit geometryCreationFailed(error);
        return false;
    }

    // Triangulate the shape
    Geometry::ShapeTriangulator triangulator;
    Geometry::TriangulationParams params;
    params.linearDeflection = 0.05; // Fine mesh for primitives
    params.angularDeflection = 0.3;
    params.colorR = 0.7f;
    params.colorG = 0.7f;
    params.colorB = 0.7f;

    auto geometry_data = triangulator.triangulate(shape, params);
    if(!geometry_data) {
        QString error = QString::fromStdString(triangulator.getLastError());
        emit geometryCreationFailed(error);
        return false;
    }

    // Send to renderer
    m_targetRenderer->setCustomGeometry(geometry_data);
    m_targetRenderer->fitToView();

    LOG_INFO("Created geometry: {} ({} vertices, {} triangles)", name.toStdString(),
             geometry_data->vertexCount(), geometry_data->indexCount() / 3);

    emit geometryCreated(name);
    return true;
}

bool GeometryCreator::createBox(qreal width, qreal height, qreal depth) {
    LOG_INFO("Creating box: {}x{}x{}", width, height, depth);

    try {
        // Create box centered at origin
        // BRepPrimAPI_MakeBox creates box with corner at origin, so we offset
        double hw = width / 2.0;
        double hh = height / 2.0;
        double hd = depth / 2.0;

        BRepPrimAPI_MakeBox box_maker(gp_Pnt(-hw, -hh, -hd), width, height, depth);
        box_maker.Build();

        if(!box_maker.IsDone()) {
            QString error = "Failed to create box shape";
            LOG_ERROR(error.toStdString());
            emit geometryCreationFailed(error);
            return false;
        }

        TopoDS_Shape shape = box_maker.Shape();
        return createFromShape(shape, QString("Box %1x%2x%3").arg(width).arg(height).arg(depth));

    } catch(const Standard_Failure& e) {
        QString error = QString("OCC error: %1").arg(e.GetMessageString());
        LOG_ERROR(error.toStdString());
        emit geometryCreationFailed(error);
        return false;
    }
}

bool GeometryCreator::createCylinder(qreal radius, qreal height) {
    LOG_INFO("Creating cylinder: radius={}, height={}", radius, height);

    try {
        // Create cylinder along Z axis, centered at origin
        gp_Ax2 axis(gp_Pnt(0, 0, -height / 2.0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeCylinder cyl_maker(axis, radius, height);
        cyl_maker.Build();

        if(!cyl_maker.IsDone()) {
            QString error = "Failed to create cylinder shape";
            LOG_ERROR(error.toStdString());
            emit geometryCreationFailed(error);
            return false;
        }

        TopoDS_Shape shape = cyl_maker.Shape();
        return createFromShape(shape, QString("Cylinder r=%1 h=%2").arg(radius).arg(height));

    } catch(const Standard_Failure& e) {
        QString error = QString("OCC error: %1").arg(e.GetMessageString());
        LOG_ERROR(error.toStdString());
        emit geometryCreationFailed(error);
        return false;
    }
}

bool GeometryCreator::createSphere(qreal radius) {
    LOG_INFO("Creating sphere: radius={}", radius);

    try {
        BRepPrimAPI_MakeSphere sphere_maker(radius);
        sphere_maker.Build();

        if(!sphere_maker.IsDone()) {
            QString error = "Failed to create sphere shape";
            LOG_ERROR(error.toStdString());
            emit geometryCreationFailed(error);
            return false;
        }

        TopoDS_Shape shape = sphere_maker.Shape();
        return createFromShape(shape, QString("Sphere r=%1").arg(radius));

    } catch(const Standard_Failure& e) {
        QString error = QString("OCC error: %1").arg(e.GetMessageString());
        LOG_ERROR(error.toStdString());
        emit geometryCreationFailed(error);
        return false;
    }
}

bool GeometryCreator::createCone(qreal bottom_radius, qreal top_radius, qreal height) {
    LOG_INFO("Creating cone: bottomR={}, topR={}, height={}", bottom_radius, top_radius, height);

    try {
        // Create cone along Z axis, centered at origin
        gp_Ax2 axis(gp_Pnt(0, 0, -height / 2.0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeCone cone_maker(axis, bottom_radius, top_radius, height);
        cone_maker.Build();

        if(!cone_maker.IsDone()) {
            QString error = "Failed to create cone shape";
            LOG_ERROR(error.toStdString());
            emit geometryCreationFailed(error);
            return false;
        }

        TopoDS_Shape shape = cone_maker.Shape();
        return createFromShape(
            shape, QString("Cone r1=%1 r2=%2 h=%3").arg(bottom_radius).arg(top_radius).arg(height));

    } catch(const Standard_Failure& e) {
        QString error = QString("OCC error: %1").arg(e.GetMessageString());
        LOG_ERROR(error.toStdString());
        emit geometryCreationFailed(error);
        return false;
    }
}

bool GeometryCreator::createTorus(qreal major_radius, qreal minor_radius) {
    LOG_INFO("Creating torus: majorR={}, minorR={}", major_radius, minor_radius);

    try {
        BRepPrimAPI_MakeTorus torus_maker(major_radius, minor_radius);
        torus_maker.Build();

        if(!torus_maker.IsDone()) {
            QString error = "Failed to create torus shape";
            LOG_ERROR(error.toStdString());
            emit geometryCreationFailed(error);
            return false;
        }

        TopoDS_Shape shape = torus_maker.Shape();
        return createFromShape(shape,
                               QString("Torus R=%1 r=%2").arg(major_radius).arg(minor_radius));

    } catch(const Standard_Failure& e) {
        QString error = QString("OCC error: %1").arg(e.GetMessageString());
        LOG_ERROR(error.toStdString());
        emit geometryCreationFailed(error);
        return false;
    }
}

} // namespace UI
} // namespace OpenGeoLab
