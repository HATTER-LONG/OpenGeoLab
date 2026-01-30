#pragma once
#include "geometry/geometry_document.hpp"
#include "geometry/part_entity.hpp"
#include "util/occ_progress.hpp"
#include <kangaroo/util/noncopyable.hpp>

class TopoDS_Face;
class TopoDS_Shape;
class TopoDS_Edge;
namespace OpenGeoLab::Geometry {

struct ShapeBuildResult {
    bool m_success{false};
    std::string m_errorMessage;
    PartEntityPtr m_rootPart;

    size_t m_vertexCount{0};
    size_t m_edgeCount{0};
    size_t m_wireCount{0};
    size_t m_faceCount{0};
    size_t m_shellCount{0};
    size_t m_solidCount{0};
    size_t m_compoundCount{0};

    [[nodiscard]] static ShapeBuildResult success(PartEntityPtr root_part) {
        ShapeBuildResult result;
        result.m_success = true;
        result.m_rootPart = std::move(root_part);
        return result;
    }

    [[nodiscard]] static ShapeBuildResult failure(const std::string& message) {
        ShapeBuildResult result;
        result.m_success = false;
        result.m_errorMessage = message;
        return result;
    }

    [[nodiscard]] size_t totalEntityCount() const {
        return m_vertexCount + m_edgeCount + m_wireCount + m_faceCount + m_shellCount +
               m_solidCount + m_compoundCount + 1; // +1 for part
    }
};

class ShapeBuilder : public Kangaroo::Util::NonCopyMoveable {
public:
    explicit ShapeBuilder(GeometryDocumentPtr document);
    ~ShapeBuilder() = default;

    [[nodiscard]] ShapeBuildResult
    buildFromShape(const TopoDS_Shape& shape,
                   const std::string& part_name = "Part",
                   Util::ProgressCallback progress_callback = Util::NO_PROGRESS_CALLBACK);

    [[nodiscard]] GeometryDocumentPtr document() const { return m_document; }

private:
    void buildSubShapes(const TopoDS_Shape& shape,
                        const GeometryEntityPtr& parent,
                        ShapeBuildResult& result,
                        Util::ProgressCallback& progress_callback);

    [[nodiscard]] GeometryEntityPtr createEntityForShape(const TopoDS_Shape& shape);

    void updateEntityCounts(const TopoDS_Shape& shape, ShapeBuildResult& result);

private:
    GeometryDocumentPtr m_document;
};
} // namespace OpenGeoLab::Geometry