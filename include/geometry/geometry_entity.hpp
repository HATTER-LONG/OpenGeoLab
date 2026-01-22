#pragma once
#include <TopoDS_Shape.hxx>
#include <kangaroo/util/noncopyable.hpp>
#include <memory>

namespace OpenGeoLab::Geometry {
class GeometryEntity;
using GeometryEntityPtr = std::shared_ptr<GeometryEntity>;
using GeometryEntityWeakPtr = std::weak_ptr<GeometryEntity>;

class GeometryEntity : public std::enable_shared_from_this<GeometryEntity>,
                       public Kangaroo::Util::NonCopyMoveable {
public:
    GeometryEntity();
    explicit GeometryEntity(const TopoDS_Shape&);
    virtual ~GeometryEntity() = default;

private:
};
} // namespace OpenGeoLab::Geometry