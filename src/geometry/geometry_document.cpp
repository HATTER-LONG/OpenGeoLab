/**
 * @file geometry_document.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry/geometry_document.hpp"
#include "geometry/shape_builder.hpp"
#include "util/logger.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>

#include <algorithm>
#include <sstream>

namespace OpenGeoLab::Geometry {

bool GeometryDocument::addEntity(const GeometryEntityPtr& entity) {
    if(!m_entityIndex.addEntity(entity)) {
        return false;
    }
    entity->setDocument(shared_from_this());
    return true;
}

bool GeometryDocument::removeEntity(EntityId entity_id) {
    const auto entity = m_entityIndex.findById(entity_id);
    if(!entity) {
        return false;
    }

    if(!m_entityIndex.removeEntity(entity_id)) {
        return false;
    }

    entity->setDocument({});
    return true;
}

void GeometryDocument::clear() { m_entityIndex.clear(); }

GeometryEntityPtr GeometryDocument::findById(EntityId entity_id) const {
    return m_entityIndex.findById(entity_id);
}

GeometryEntityPtr GeometryDocument::findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const {
    return m_entityIndex.findByUIDAndType(entity_uid, entity_type);
}

GeometryEntityPtr GeometryDocument::findByShape(const TopoDS_Shape& shape) const {
    return m_entityIndex.findByShape(shape);
}

[[nodiscard]] size_t GeometryDocument::entityCount() const { return m_entityIndex.entityCount(); }

[[nodiscard]] size_t GeometryDocument::entityCountByType(EntityType entity_type) const {
    return m_entityIndex.entityCountByType(entity_type);
}
bool GeometryDocument::addChildEdge(EntityId parent_id, EntityId child_id) {
    if(parent_id == INVALID_ENTITY_ID || child_id == INVALID_ENTITY_ID) {
        return false;
    }
    if(parent_id == child_id) {
        return false;
    }

    const auto parent = findById(parent_id);
    const auto child = findById(child_id);
    if(!parent || !child) {
        return false;
    }

    // Enforce type-level relationship constraints.
    if(!parent->canAddChildType(child->entityType()) ||
       !child->canAddParentType(parent->entityType())) {
        return false;
    }

    const bool inserted = parent->addChildNoSync(child_id);
    if(!inserted) {
        return false;
    }

    (void)child->addParentNoSync(parent_id);
    return true;
}

bool GeometryDocument::removeChildEdge(EntityId parent_id, EntityId child_id) {
    if(parent_id == INVALID_ENTITY_ID || child_id == INVALID_ENTITY_ID) {
        return false;
    }
    if(parent_id == child_id) {
        return false;
    }

    const auto parent = findById(parent_id);
    if(!parent) {
        return false;
    }

    const bool erased = parent->removeChildNoSync(child_id);
    if(!erased) {
        return false;
    }

    // Best-effort: if child already expired, local removal is enough.
    if(const auto child = findById(child_id)) {
        (void)child->removeParentNoSync(parent_id);
    }

    return true;
}

// =============================================================================
// Part Management
// =============================================================================

std::vector<PartEntityPtr> GeometryDocument::getAllParts() const {
    std::vector<PartEntityPtr> parts;
    auto all_entities = m_entityIndex.snapshotEntities();

    for(const auto& entity : all_entities) {
        if(entity && entity->entityType() == EntityType::Part) {
            if(auto part = std::dynamic_pointer_cast<PartEntity>(entity)) {
                parts.push_back(part);
            }
        }
    }

    return parts;
}

size_t GeometryDocument::partCount() const {
    return m_entityIndex.entityCountByType(EntityType::Part);
}

PartEntityPtr GeometryDocument::findPartByName(const std::string& name) const {
    auto all_entities = m_entityIndex.snapshotEntities();

    for(const auto& entity : all_entities) {
        if(entity && entity->entityType() == EntityType::Part && entity->name() == name) {
            return std::dynamic_pointer_cast<PartEntity>(entity);
        }
    }

    return nullptr;
}

PartEntityPtr GeometryDocument::appendShape(const TopoDS_Shape& shape,
                                            const std::string& part_name,
                                            DocumentProgressCallback progress_callback) {
    if(shape.IsNull()) {
        LOG_ERROR("Cannot append null shape");
        return nullptr;
    }

    // Generate unique name if not provided
    std::string actual_name = part_name.empty() ? generateUniquePartName("Part") : part_name;

    // Ensure uniqueness
    if(findPartByName(actual_name)) {
        actual_name = generateUniquePartName(actual_name);
    }

    // Use ShapeBuilder to create the entity hierarchy
    ShapeBuilder builder(shared_from_this());
    auto options = ShapeBuildOptions::forRendering();

    auto result =
        builder.buildFromShape(shape, actual_name, options, [&](double p, const std::string& msg) {
            if(progress_callback) {
                return progress_callback(p, msg);
            }
            return true;
        });

    if(!result.m_success) {
        LOG_ERROR("Failed to build shape: {}", result.m_errorMessage);
        return nullptr;
    }

    LOG_INFO("Appended part '{}' with {} entities", actual_name, result.totalEntityCount());
    return result.m_rootPart;
}

PartEntityPtr
GeometryDocument::createBox(double dx, double dy, double dz, const std::string& part_name) {
    try {
        BRepPrimAPI_MakeBox maker(dx, dy, dz);
        maker.Build();
        if(!maker.IsDone()) {
            LOG_ERROR("Failed to create box");
            return nullptr;
        }
        return appendShape(maker.Shape(), part_name);
    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC error creating box: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr GeometryDocument::createSphere(double radius, const std::string& part_name) {
    try {
        BRepPrimAPI_MakeSphere maker(radius);
        maker.Build();
        if(!maker.IsDone()) {
            LOG_ERROR("Failed to create sphere");
            return nullptr;
        }
        return appendShape(maker.Shape(), part_name);
    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC error creating sphere: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr
GeometryDocument::createCylinder(double radius, double height, const std::string& part_name) {
    try {
        BRepPrimAPI_MakeCylinder maker(radius, height);
        maker.Build();
        if(!maker.IsDone()) {
            LOG_ERROR("Failed to create cylinder");
            return nullptr;
        }
        return appendShape(maker.Shape(), part_name);
    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC error creating cylinder: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

PartEntityPtr GeometryDocument::createCone(double radius1,
                                           double radius2,
                                           double height,
                                           const std::string& part_name) {
    try {
        BRepPrimAPI_MakeCone maker(radius1, radius2, height);
        maker.Build();
        if(!maker.IsDone()) {
            LOG_ERROR("Failed to create cone");
            return nullptr;
        }
        return appendShape(maker.Shape(), part_name);
    } catch(const Standard_Failure& e) {
        LOG_ERROR("OCC error creating cone: {}",
                  e.GetMessageString() ? e.GetMessageString() : "Unknown");
        return nullptr;
    }
}

// =============================================================================
// Render Data Generation
// =============================================================================

DocumentRenderDataPtr
GeometryDocument::generateRenderData(const TessellationParams& params,
                                     DocumentProgressCallback progress_callback) {
    auto render_data = std::make_shared<DocumentRenderData>();
    auto parts = getAllParts();

    if(parts.empty()) {
        return render_data;
    }

    size_t part_index = 0;
    for(const auto& part : parts) {
        if(progress_callback) {
            double progress = static_cast<double>(part_index) / parts.size();
            if(!progress_callback(progress, "Generating render data for " + part->name())) {
                return nullptr; // Cancelled
            }
        }

        auto part_data = generatePartRenderData(part->entityId(), params);
        if(part_data) {
            // Assign distinct color based on index
            part_data->m_baseColor = RenderColor::fromIndex(part_index);
            render_data->m_parts.push_back(part_data);
        }

        ++part_index;
    }

    render_data->updateSceneBoundingBox();

    if(progress_callback) {
        progress_callback(1.0, "Render data generation complete");
    }

    return render_data;
}

PartRenderDataPtr GeometryDocument::generatePartRenderData(EntityId part_id,
                                                           const TessellationParams& params) {
    auto entity = findById(part_id);
    if(!entity || entity->entityType() != EntityType::Part) {
        return nullptr;
    }

    auto part = std::dynamic_pointer_cast<PartEntity>(entity);
    if(!part) {
        return nullptr;
    }

    // Create a temporary builder to generate render data
    ShapeBuilder builder(shared_from_this());
    auto options = ShapeBuildOptions();
    options.m_generateRenderData = true;
    options.m_tessellation = params;

    // We need to generate render data without rebuilding the hierarchy
    // Use the shape directly
    auto result = builder.buildFromShape(part->shape(), part->name(), options, nullptr);

    return result.m_renderData;
}

// =============================================================================
// Mesh Metadata Generation
// =============================================================================

DocumentMeshMetadataPtr
GeometryDocument::generateMeshMetadata(DocumentProgressCallback progress_callback) {
    auto mesh_data = std::make_shared<DocumentMeshMetadata>();
    auto parts = getAllParts();

    if(parts.empty()) {
        return mesh_data;
    }

    size_t part_index = 0;
    for(const auto& part : parts) {
        if(progress_callback) {
            double progress = static_cast<double>(part_index) / parts.size();
            if(!progress_callback(progress, "Generating mesh metadata for " + part->name())) {
                return nullptr; // Cancelled
            }
        }

        auto part_meta = generatePartMeshMetadata(part->entityId());
        if(part_meta) {
            mesh_data->m_parts.push_back(part_meta);
        }

        ++part_index;
    }

    mesh_data->updateSceneBoundingBox();

    if(progress_callback) {
        progress_callback(1.0, "Mesh metadata generation complete");
    }

    return mesh_data;
}

PartMeshMetadataPtr GeometryDocument::generatePartMeshMetadata(EntityId part_id) {
    auto entity = findById(part_id);
    if(!entity || entity->entityType() != EntityType::Part) {
        return nullptr;
    }

    auto part = std::dynamic_pointer_cast<PartEntity>(entity);
    if(!part) {
        return nullptr;
    }

    // Create a temporary builder to generate mesh metadata
    ShapeBuilder builder(shared_from_this());
    auto options = ShapeBuildOptions();
    options.m_generateMeshMetadata = true;

    auto result = builder.buildFromShape(part->shape(), part->name(), options, nullptr);

    return result.m_meshMetadata;
}

// =============================================================================
// Document Bounding Box
// =============================================================================

BoundingBox3D GeometryDocument::sceneBoundingBox() const {
    BoundingBox3D scene_box;
    auto parts = getAllParts();

    for(const auto& part : parts) {
        if(part) {
            scene_box.expand(part->boundingBox());
        }
    }

    return scene_box;
}

// =============================================================================
// Private Helpers
// =============================================================================

std::string GeometryDocument::generateUniquePartName(const std::string& base_name) {
    std::string name = base_name;
    size_t counter = ++m_partNameCounter;

    while(findPartByName(name)) {
        std::ostringstream oss;
        oss << base_name << "_" << counter++;
        name = oss.str();
    }

    return name;
}

// =============================================================================
// GeometryDocumentManager Implementation
// =============================================================================

GeometryDocumentManager& GeometryDocumentManager::instance() {
    static GeometryDocumentManager s_instance;
    return s_instance;
}

GeometryDocumentPtr GeometryDocumentManager::currentDocument() {
    if(!m_currentDocument) {
        return newDocument();
    }
    return m_currentDocument;
}

GeometryDocumentPtr GeometryDocumentManager::newDocument() {
    // Clear existing document if any
    if(m_currentDocument) {
        m_currentDocument->clear();
    }

    m_currentDocument = GeometryDocument::create();
    LOG_INFO("Created new geometry document");
    return m_currentDocument;
}

size_t GeometryDocumentManager::partCount() const {
    if(!m_currentDocument) {
        return 0;
    }
    return m_currentDocument->partCount();
}

void GeometryDocumentManager::clearCurrentDocument() {
    if(m_currentDocument) {
        m_currentDocument->clear();
        LOG_INFO("Cleared current document");
    }
}

} // namespace OpenGeoLab::Geometry
