#pragma once

#include "render/render_data.hpp"

#include <memory>
#include <vector>

namespace OpenGeoLab::Geometry {
class EntityIndex;
class EntityRelationshipIndex;
class GeometryEntityImpl;
using GeometryEntityImplPtr = std::shared_ptr<GeometryEntityImpl>;
} // namespace OpenGeoLab::Geometry

namespace OpenGeoLab::Render {

struct GeometryRenderInput {
    const Geometry::EntityIndex& m_entityIndex;
    const Geometry::EntityRelationshipIndex& m_relationshipIndex;
    TessellationOptions m_options;
};

class GeometryRenderBuilder {
public:
    static bool build(RenderData& render_data, const GeometryRenderInput& input);

private:
    static DrawRange generateFaceMesh(RenderData& rd,
                                      const Geometry::GeometryEntityImplPtr& entity,
                                      Geometry::EntityUID owner_part_uid,
                                      const TessellationOptions& opts);

    static DrawRange generateEdgeMesh(RenderData& rd,
                                      const Geometry::GeometryEntityImplPtr& entity,
                                      const TessellationOptions& opts);

    static DrawRange generateVertexMesh(RenderData& rd,
                                        const Geometry::GeometryEntityImplPtr& entity);
};

} // namespace OpenGeoLab::Render
