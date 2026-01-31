#pragma once
#include "geometry/geometry_document.hpp"
#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Geometry {

class GeometryDocumentManager : public Kangaroo::Util::NonCopyMoveable {
public:
    ~GeometryDocumentManager() = default;

    [[nodiscard]] virtual GeometryDocumentPtr currentDocument() = 0;

    [[nodiscard]] virtual GeometryDocumentPtr newDocument() = 0;

protected:
    GeometryDocumentManager() = default;
};

class IGeoDocumentManagerSingletonFactory
    : public Kangaroo::Util::FactoryTraits<IGeoDocumentManagerSingletonFactory,
                                           GeometryDocumentManager> {
public:
    IGeoDocumentManagerSingletonFactory() = default;
    virtual ~IGeoDocumentManagerSingletonFactory() = default;

    virtual tObjectSharedPtr instance() const = 0;
};

} // namespace OpenGeoLab::Geometry
