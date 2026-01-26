/**
 * @file primitive_factory.cpp
 * @brief Implementation of primitive geometry creation factory
 */

#include "geometry/primitive_factory.hpp"
#include "geometry/geometry_builder.hpp"
#include "util/logger.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakeWedge.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>

namespace OpenGeoLab::Geometry {

GeometryDocumentPtr PrimitiveFactory::ensureDocument(GeometryDocumentPtr document) {
    if(document) {
        return document;
    }

    auto& manager = GeometryDocumentManager::instance();
    auto current = manager.currentDocument();
    if(current) {
        return current;
    }

    return manager.newDocument();
}

PartEntityPtr PrimitiveFactory::buildPart(const TopoDS_Shape& shape,
                                          const std::string& name,
                                          GeometryDocumentPtr document) {
    if(shape.IsNull()) {
        LOG_ERROR("Cannot build part from null shape");
        return nullptr;
    }

    auto doc = ensureDocument(std::move(document));
    if(!doc) {
        LOG_ERROR("No document available for part creation");
        return nullptr;
    }

    GeometryBuilder builder(doc);
    auto result = builder.buildFromShape(shape, name, nullptr);

    if(!result.m_success) {
        LOG_ERROR("Failed to build part '{}': {}", name, result.m_errorMessage);
        return nullptr;
    }

    LOG_DEBUG("Created primitive part '{}' with {} entities", name, doc->entityCount());
    return result.m_partEntity;
}

PartEntityPtr
PrimitiveFactory::createBox(double dx, double dy, double dz, GeometryDocumentPtr document) {
    if(dx <= 0 || dy <= 0 || dz <= 0) {
        LOG_ERROR("Box dimensions must be positive: dx={}, dy={}, dz={}", dx, dy, dz);
        return nullptr;
    }

    try {
        // Create box centered at origin
        gp_Pnt p1(-dx / 2.0, -dy / 2.0, -dz / 2.0);
        gp_Pnt p2(dx / 2.0, dy / 2.0, dz / 2.0);
        BRepPrimAPI_MakeBox maker(p1, p2);
        maker.Build();

        if(!maker.IsDone()) {
            LOG_ERROR("BRepPrimAPI_MakeBox failed");
            return nullptr;
        }

        return buildPart(maker.Shape(), "Box", std::move(document));

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating box: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr
PrimitiveFactory::createBox(const Point3D& p1, const Point3D& p2, GeometryDocumentPtr document) {
    try {
        gp_Pnt gp1(p1.m_x, p1.m_y, p1.m_z);
        gp_Pnt gp2(p2.m_x, p2.m_y, p2.m_z);
        BRepPrimAPI_MakeBox maker(gp1, gp2);
        maker.Build();

        if(!maker.IsDone()) {
            LOG_ERROR("BRepPrimAPI_MakeBox failed");
            return nullptr;
        }

        return buildPart(maker.Shape(), "Box", std::move(document));

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating box: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr PrimitiveFactory::createSphere(double radius, GeometryDocumentPtr document) {
    if(radius <= 0) {
        LOG_ERROR("Sphere radius must be positive: {}", radius);
        return nullptr;
    }

    try {
        BRepPrimAPI_MakeSphere maker(radius);
        maker.Build();

        if(!maker.IsDone()) {
            LOG_ERROR("BRepPrimAPI_MakeSphere failed");
            return nullptr;
        }

        return buildPart(maker.Shape(), "Sphere", std::move(document));

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating sphere: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr
PrimitiveFactory::createSphere(const Point3D& center, double radius, GeometryDocumentPtr document) {
    if(radius <= 0) {
        LOG_ERROR("Sphere radius must be positive: {}", radius);
        return nullptr;
    }

    try {
        gp_Pnt center_pnt(center.m_x, center.m_y, center.m_z);
        BRepPrimAPI_MakeSphere maker(center_pnt, radius);
        maker.Build();

        if(!maker.IsDone()) {
            LOG_ERROR("BRepPrimAPI_MakeSphere failed");
            return nullptr;
        }

        return buildPart(maker.Shape(), "Sphere", std::move(document));

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating sphere: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr
PrimitiveFactory::createCylinder(double radius, double height, GeometryDocumentPtr document) {
    if(radius <= 0 || height <= 0) {
        LOG_ERROR("Cylinder radius and height must be positive: r={}, h={}", radius, height);
        return nullptr;
    }

    try {
        // Create cylinder centered along Z axis
        gp_Ax2 axis(gp_Pnt(0, 0, -height / 2.0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeCylinder maker(axis, radius, height);
        maker.Build();

        if(!maker.IsDone()) {
            LOG_ERROR("BRepPrimAPI_MakeCylinder failed");
            return nullptr;
        }

        return buildPart(maker.Shape(), "Cylinder", std::move(document));

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating cylinder: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr PrimitiveFactory::createCone(double radius_bottom,
                                           double radius_top,
                                           double height,
                                           GeometryDocumentPtr document) {
    if(height <= 0) {
        LOG_ERROR("Cone height must be positive: {}", height);
        return nullptr;
    }

    if(radius_bottom < 0 || radius_top < 0) {
        LOG_ERROR("Cone radii must be non-negative: r1={}, r2={}", radius_bottom, radius_top);
        return nullptr;
    }

    if(radius_bottom == radius_top) {
        LOG_WARN("Cone with equal radii is a cylinder, use createCylinder instead");
        return createCylinder(radius_bottom, height, std::move(document));
    }

    try {
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeCone maker(axis, radius_bottom, radius_top, height);
        maker.Build();

        if(!maker.IsDone()) {
            LOG_ERROR("BRepPrimAPI_MakeCone failed");
            return nullptr;
        }

        return buildPart(maker.Shape(), "Cone", std::move(document));

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating cone: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr PrimitiveFactory::createTorus(double major_radius,
                                            double minor_radius,
                                            GeometryDocumentPtr document) {
    if(minor_radius <= 0) {
        LOG_ERROR("Torus minor radius must be positive: {}", minor_radius);
        return nullptr;
    }

    if(major_radius <= minor_radius) {
        LOG_ERROR("Torus major radius must be greater than minor radius: R={}, r={}", major_radius,
                  minor_radius);
        return nullptr;
    }

    try {
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeTorus maker(axis, major_radius, minor_radius);
        maker.Build();

        if(!maker.IsDone()) {
            LOG_ERROR("BRepPrimAPI_MakeTorus failed");
            return nullptr;
        }

        return buildPart(maker.Shape(), "Torus", std::move(document));

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating torus: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr PrimitiveFactory::createWedge(
    double dx, double dy, double dz, double ltx, GeometryDocumentPtr document) {
    if(dx <= 0 || dy <= 0 || dz <= 0) {
        LOG_ERROR("Wedge dimensions must be positive: dx={}, dy={}, dz={}", dx, dy, dz);
        return nullptr;
    }

    if(ltx < 0 || ltx > dx) {
        LOG_ERROR("Wedge top length must be in [0, dx]: ltx={}, dx={}", ltx, dx);
        return nullptr;
    }

    try {
        BRepPrimAPI_MakeWedge maker(dx, dy, dz, ltx);
        maker.Build();

        if(!maker.IsDone()) {
            LOG_ERROR("BRepPrimAPI_MakeWedge failed");
            return nullptr;
        }

        return buildPart(maker.Shape(), "Wedge", std::move(document));

    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC exception creating wedge: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

} // namespace OpenGeoLab::Geometry
