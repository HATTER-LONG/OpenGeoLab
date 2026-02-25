#include "app/select_manager_service.hpp"
#include "geometry/geometry_document.hpp"
#include "mesh/mesh_document.hpp"
#include "render/render_select_manager.hpp"
#include "util/logger.hpp"
#include <QtCore/QMetaObject>

namespace OpenGeoLab::App {

namespace {

QString convertPickResultTypeToString(const Render::PickResult& result) {
    if(Render::isMeshDomain(result.m_type)) {
        const auto mesh_elem_type_opt =
            meshElementTypeToString(Render::toMeshElementType(result.m_type));
        if(mesh_elem_type_opt.has_value()) {
            return QString::fromStdString(mesh_elem_type_opt.value());
        }
    } else if(Render::isGeometryDomain(result.m_type)) {
        const auto geom_type_opt =
            Geometry::entityTypeToString(Render::toGeometryType(result.m_type));
        if(geom_type_opt.has_value()) {
            return QString::fromStdString(geom_type_opt.value());
        }
    }
    return "Invalid";
}

Render::RenderEntityType convertStringToRenderEntityType(const QString& type_str) {
    const auto geom_type_opt = Geometry::entityTypeFromString(type_str.toStdString());
    if(geom_type_opt.has_value()) {
        return Render::toRenderEntityType(geom_type_opt.value());
    }

    const auto mesh_elem_type_opt = Mesh::meshElementTypeFromString(type_str.toStdString());
    if(mesh_elem_type_opt.has_value()) {
        return Render::toRenderEntityType(mesh_elem_type_opt.value());
    }

    throw std::invalid_argument("Unknown entity type string: " + type_str.toStdString());
}

QVariantMap toVariantMap(const Geometry::EntityKey& key) {
    QVariantMap map;
    map["id"] = static_cast<qulonglong>(key.m_id);
    map["uid"] = static_cast<qulonglong>(key.m_uid);
    const auto type_opt = Geometry::entityTypeToString(key.m_type);
    map["type"] = type_opt.has_value() ? QString::fromStdString(type_opt.value()) : "Invalid";
    return map;
}

QVariantList collectRelated(Geometry::GeometryDocumentPtr doc,
                            Geometry::EntityUID uid,
                            Geometry::EntityType source,
                            Geometry::EntityType related) {
    QVariantList out;
    const auto keys = doc->findRelatedEntities(uid, source, related);
    out.reserve(static_cast<qsizetype>(keys.size()));
    for(const auto& key : keys) {
        out.push_back(toVariantMap(key));
    }
    return out;
}
} // namespace

SelectManagerService::SelectManagerService(QObject* parent) : QObject(parent) {
    m_selectSettingsConn = Render::RenderSelectManager::instance().subscribePickSettingsChanged(
        [this](Render::RenderEntityTypeMask types) {
            QMetaObject::invokeMethod(
                this, [this, types]() { emit selectModeChanged(static_cast<uint32_t>(types)); },
                Qt::QueuedConnection);
        });

    m_selectionConn = Render::RenderSelectManager::instance().subscribeSelectionChanged(
        [this](Render::PickResult pr, Render::SelectionChangeAction action) {
            QMetaObject::invokeMethod(
                this,
                [this, pr, action]() {
                    switch(action) {
                    case Render::SelectionChangeAction::Added: {
                        const auto type_str = convertPickResultTypeToString(pr);
                        emit entitySelected(static_cast<uint64_t>(pr.m_uid), type_str);
                        break;
                    }
                    case Render::SelectionChangeAction::Removed: {
                        const auto type_str = convertPickResultTypeToString(pr);
                        emit entityRemoved(static_cast<uint64_t>(pr.m_uid), type_str);
                        break;
                    }
                    case Render::SelectionChangeAction::Cleared:
                        emit selectionCleared();
                        break;
                    }
                },
                Qt::QueuedConnection);
        });
    m_selectionEnable =
        Render::RenderSelectManager::instance().subscribePickEnabledChanged([this](bool enabled) {
            QMetaObject::invokeMethod(
                this, [this, enabled]() { emit selectModeActivated(enabled); },
                Qt::QueuedConnection);
        });
}
SelectManagerService::~SelectManagerService() = default;

void SelectManagerService::activateSelectMode(uint32_t select_types) {
    auto& select = Render::RenderSelectManager::instance();
    select.setPickEnabled(true);
    select.setPickTypes(static_cast<Render::RenderEntityTypeMask>(select_types));
}

void SelectManagerService::deactivateSelectMode() {
    Render::RenderSelectManager::instance().setPickEnabled(false);
}

bool SelectManagerService::isSelectEnabled() const {
    return Render::RenderSelectManager::instance().isPickEnabled();
}

void SelectManagerService::clearSelection() {
    Render::RenderSelectManager::instance().clearSelection();
}

void SelectManagerService::selectEntity(uint64_t entity_uid, const QString& entity_type) {
    try {
        auto entity_type_enum = convertStringToRenderEntityType(entity_type);

        Render::RenderSelectManager::instance().addSelection(
            static_cast<Mesh::MeshElementUID>(entity_uid), entity_type_enum);
    } catch(...) {
        LOG_ERROR("SelectManagerService::selectEntity: Invalid entity '{}:{}'",
                  entity_type.toStdString(), entity_uid);
        return;
    }
    return;
}

void SelectManagerService::removeEntity(uint64_t entity_uid, const QString& entity_type) {
    try {
        auto entity_type_enum = convertStringToRenderEntityType(entity_type);

        Render::RenderSelectManager::instance().removeSelection(
            static_cast<Mesh::MeshElementUID>(entity_uid), entity_type_enum);

    } catch(...) {
        LOG_ERROR("SelectManagerService::removeEntity: Invalid entity '{}:{}'",
                  entity_type.toStdString(), entity_uid);
        return;
    }
    return;
}

QVector<QPair<uint64_t, QString>> SelectManagerService::currentSelections() const {
    QVector<QPair<uint64_t, QString>> out;
    const auto sels = Render::RenderSelectManager::instance().selections();
    for(const auto& pr : sels) {
        out.append({static_cast<uint64_t>(pr.m_uid), convertPickResultTypeToString(pr)});
    }
    return out;
}

QVariantMap SelectManagerService::queryEntityInfo(uint64_t entity_uid,
                                                  const QString& entity_type) const {
    QVariantMap result;
    result["uid"] = static_cast<qulonglong>(entity_uid);
    result["type"] = entity_type;

    Render::RenderEntityType render_type = Render::RenderEntityType::None;
    try {
        render_type = convertStringToRenderEntityType(entity_type);
    } catch(...) {
        result["found"] = false;
        result["error"] = "unknown_entity_type";
        return result;
    }

    if(Render::isGeometryDomain(render_type)) {
        const auto geom_type = Render::toGeometryType(render_type);
        auto doc = GeoDocumentInstance;
        const auto entity = doc->findByUIDAndType(entity_uid, geom_type);
        if(!entity) {
            result["found"] = false;
            return result;
        }

        result["found"] = true;
        result["id"] = static_cast<qulonglong>(entity->entityId());
        result["uid"] = static_cast<qulonglong>(entity->entityUID());

        QVariantMap related;
        related["part"] = collectRelated(doc, entity_uid, geom_type, Geometry::EntityType::Part);
        related["solid"] = collectRelated(doc, entity_uid, geom_type, Geometry::EntityType::Solid);
        related["wire"] = collectRelated(doc, entity_uid, geom_type, Geometry::EntityType::Wire);
        related["face"] = collectRelated(doc, entity_uid, geom_type, Geometry::EntityType::Face);
        related["edge"] = collectRelated(doc, entity_uid, geom_type, Geometry::EntityType::Edge);
        related["vertex"] =
            collectRelated(doc, entity_uid, geom_type, Geometry::EntityType::Vertex);
        result["related"] = related;
        return result;
    }

    if(render_type == Render::RenderEntityType::MeshNode) {
        auto mesh = MeshDocumentInstance;
        try {
            const auto node = mesh->findNodeById(entity_uid);
            result["found"] = true;
            result["position"] = QVariantList{node.x(), node.y(), node.z()};
        } catch(...) {
            result["found"] = false;
        }
        return result;
    }

    if(render_type == Render::RenderEntityType::MeshLine ||
       render_type == Render::RenderEntityType::MeshTriangle ||
       render_type == Render::RenderEntityType::MeshQuad4 ||
       render_type == Render::RenderEntityType::MeshTetra4 ||
       render_type == Render::RenderEntityType::MeshHexa8 ||
       render_type == Render::RenderEntityType::MeshPrism6 ||
       render_type == Render::RenderEntityType::MeshPyramid5) {
        auto mesh = MeshDocumentInstance;
        try {
            const auto element_type = Render::toMeshElementType(render_type);
            const Mesh::MeshElementRef ref{entity_uid, element_type};
            const auto element = mesh->findElementByRef(ref);
            result["found"] = true;
            result["id"] = static_cast<qulonglong>(element.elementId());
            QVariantList node_ids;
            const auto ids = element.nodeIds();
            node_ids.reserve(static_cast<qsizetype>(ids.size()));
            for(const auto node_id : ids) {
                node_ids.push_back(static_cast<qulonglong>(node_id));
            }
            result["node_ids"] = node_ids;
        } catch(...) {
            result["found"] = false;
        }
        return result;
    }

    result["found"] = false;
    return result;
}

} // namespace OpenGeoLab::App