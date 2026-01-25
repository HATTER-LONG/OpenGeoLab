#include "geometry/entity_builder.hpp"

#include "geometry/comp_solid_entity.hpp"
#include "geometry/compound_entity.hpp"
#include "geometry/edge_entity.hpp"
#include "geometry/face_entity.hpp"
#include "geometry/part_entity.hpp"
#include "geometry/shell_entity.hpp"
#include "geometry/solid_entity.hpp"
#include "geometry/vertex_entity.hpp"
#include "geometry/wire_entity.hpp"

#include <BRep_Builder.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shape.hxx>

#include <unordered_map>
#include <utility>
#include <vector>

namespace OpenGeoLab::Geometry {
namespace {

struct CacheEntry {
    GeometryEntityPtr m_entity;
    bool m_built{false};
};

using CacheMap = std::unordered_map<TopoDS_Shape, CacheEntry, TopTools_ShapeMapHasher>;

[[nodiscard]] GeometryEntityPtr createEntityForShape(const TopoDS_Shape& shape) {
    if(shape.IsNull()) {
        return nullptr;
    }

    switch(GeometryEntity::detectEntityType(shape)) {
    case EntityType::Vertex:
        return std::make_shared<VertexEntity>(TopoDS::Vertex(shape));
    case EntityType::Edge:
        return std::make_shared<EdgeEntity>(TopoDS::Edge(shape));
    case EntityType::Wire:
        return std::make_shared<WireEntity>(TopoDS::Wire(shape));
    case EntityType::Face:
        return std::make_shared<FaceEntity>(TopoDS::Face(shape));
    case EntityType::Shell:
        return std::make_shared<ShellEntity>(TopoDS::Shell(shape));
    case EntityType::Solid:
        return std::make_shared<SolidEntity>(TopoDS::Solid(shape));
    case EntityType::CompSolid:
        return std::make_shared<CompSolidEntity>(TopoDS::CompSolid(shape));
    case EntityType::Compound:
        return std::make_shared<CompoundEntity>(TopoDS::Compound(shape));
    case EntityType::Part:
        return std::make_shared<PartEntity>(shape);
    default:
        return nullptr;
    }
}

[[nodiscard]] GeometryEntityPtr getOrCreate(const TopoDS_Shape& shape, CacheMap& cache) {
    if(shape.IsNull()) {
        return nullptr;
    }

    auto it = cache.find(shape);
    if(it != cache.end()) {
        return it->second.m_entity;
    }

    CacheEntry entry;
    entry.m_entity = createEntityForShape(shape);
    auto [ins_it, _] = cache.emplace(shape, std::move(entry));
    return ins_it->second.m_entity;
}

void buildGraphNonRecursive(const GeometryEntityPtr& root_entity,
                            const TopoDS_Shape& root_shape,
                            CacheMap& cache) {
    if(!root_entity || root_shape.IsNull()) {
        return;
    }

    struct WorkItem {
        GeometryEntityPtr m_entity;
        TopoDS_Shape m_shape;
    };

    std::vector<WorkItem> stack;
    stack.push_back(WorkItem{root_entity, root_shape});

    while(!stack.empty()) {
        auto item = std::move(stack.back());
        stack.pop_back();

        if(!item.m_entity || item.m_shape.IsNull()) {
            continue;
        }

        auto cache_it = cache.find(item.m_shape);
        if(cache_it != cache.end() && cache_it->second.m_built) {
            continue;
        }
        if(cache_it != cache.end()) {
            cache_it->second.m_built = true;
        }

        for(TopoDS_Iterator it(item.m_shape); it.More(); it.Next()) {
            const TopoDS_Shape child_shape = it.Value();
            if(child_shape.IsNull()) {
                continue;
            }

            auto child_entity = getOrCreate(child_shape, cache);
            if(!child_entity) {
                continue;
            }

            item.m_entity->addChild(child_entity);
            stack.push_back(WorkItem{child_entity, child_shape});
        }
    }
}

} // namespace

GeometryEntityPtr buildCompoundModelWithParts(const TopoDS_Shape& model_shape,
                                              const std::string& part_name_prefix) {
    if(model_shape.IsNull()) {
        return nullptr;
    }

    // Normalize to a root compound to simplify the UI model.
    TopoDS_Compound root_compound;
    BRep_Builder builder;
    builder.MakeCompound(root_compound);

    std::vector<TopoDS_Shape> part_shapes;
    part_shapes.reserve(8);

    if(model_shape.ShapeType() == TopAbs_COMPOUND) {
        for(TopoDS_Iterator it(model_shape); it.More(); it.Next()) {
            if(!it.Value().IsNull()) {
                part_shapes.push_back(it.Value());
            }
        }
    } else {
        part_shapes.push_back(model_shape);
    }

    for(const auto& part_shape : part_shapes) {
        builder.Add(root_compound, part_shape);
    }

    auto root_entity = std::make_shared<CompoundEntity>(root_compound);
    root_entity->setName("Model");

    CacheMap cache;
    cache.emplace(root_compound, CacheEntry{root_entity, true});

    // Each top-level shape becomes an independent part.
    for(size_t i = 0; i < part_shapes.size(); ++i) {
        const auto& part_shape = part_shapes[i];

        // Force PartEntity for top-level components so UI has a stable type.
        auto part_entity = std::make_shared<PartEntity>(part_shape);
        cache.emplace(part_shape, CacheEntry{part_entity, false});

        if(!part_entity) {
            continue;
        }

        part_entity->setName(part_name_prefix + " " + std::to_string(i + 1));
        root_entity->addChild(part_entity);

        buildGraphNonRecursive(part_entity, part_shape, cache);
    }

    return root_entity;
}

} // namespace OpenGeoLab::Geometry
