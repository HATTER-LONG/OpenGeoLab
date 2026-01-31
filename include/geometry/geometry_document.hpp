#pragma once
#include <kangaroo/util/noncopyable.hpp>
#include <memory>
namespace OpenGeoLab::Geometry {
class GeometryDocument;
using GeometryDocumentPtr = std::shared_ptr<GeometryDocument>;

class GeometryDocument : public Kangaroo::Util::NonCopyMoveable {
public:
    GeometryDocument() = default;
    virtual ~GeometryDocument() = default;

    [[nodiscard]] virtual bool buildShapes() = 0;
    // support get render context
    [[nodiscard]] virtual void* getRenderContext() const = 0;
};

} // namespace OpenGeoLab::Geometry