/**
 * @file pick_manager.cpp
 * @brief Implementation of PickManager QML service
 */

#include "app/pick_manager.hpp"
#include "geometry/geometry_document_manager.hpp"
#include "geometry/geometry_types.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/component_factory.hpp>

#include <QMetaObject>

namespace OpenGeoLab::App {

PickManager* PickManager::s_instance = nullptr;

PickManager::PickManager(QObject* parent) : QObject(parent) {
    s_instance = this;

    // Initialize default context
    m_contexts["default"] = SelectionContext{};

    // Subscribe to SelectManager changes
    auto& select_manager = Render::SelectManager::instance();

    m_pickSettingsConn = select_manager.subscribePickSettingsChanged([this]() {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                syncFromSelectManager();
                emit pickModeActiveChanged();
                emit selectedTypeChanged();
            },
            Qt::QueuedConnection);
    });

    m_selectionConn = select_manager.subscribeSelectionChanged([this]() {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                syncFromSelectManager();
                emit selectedEntitiesChanged();
                emit selectionChanged(m_contextKey, selectedEntities());
            },
            Qt::QueuedConnection);
    });

    LOG_TRACE("PickManager created");
}

PickManager::~PickManager() {
    s_instance = nullptr;
    LOG_TRACE("PickManager destroyed");
}

PickManager* PickManager::instance() { return s_instance; }

// =============================================================================
// Property Accessors
// =============================================================================

QString PickManager::selectedType() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return currentContext().m_selectedType;
}

bool PickManager::pickModeActive() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return currentContext().m_pickModeActive;
}

QVariantList PickManager::selectedEntities() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    QVariantList result;

    const auto& ctx = currentContext();
    for(const auto& sel : ctx.m_selections) {
        QVariantMap entity;
        entity["type"] = entityTypeToString(sel.m_type);
        entity["uid"] = static_cast<int>(sel.m_uid);
        result.append(entity);
    }

    return result;
}

QString PickManager::contextKey() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_contextKey;
}

void PickManager::setContextKey(const QString& key) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_contextKey == key) {
            return;
        }
        m_contextKey = key;

        // Create context if it doesn't exist
        const std::string key_str = key.toStdString();
        if(m_contexts.find(key_str) == m_contexts.end()) {
            m_contexts[key_str] = SelectionContext{};
        }
    }

    syncToSelectManager();

    emit contextKeyChanged();
    emit selectedTypeChanged();
    emit pickModeActiveChanged();
    emit selectedEntitiesChanged();
}

bool PickManager::expandPartSolidSelection() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_expandPartSolidSelection;
}

void PickManager::setExpandPartSolidSelection(bool expand) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_expandPartSolidSelection == expand) {
            return;
        }
        m_expandPartSolidSelection = expand;
    }
    emit expandPartSolidSelectionChanged();
}

// =============================================================================
// Q_INVOKABLE Methods
// =============================================================================

void PickManager::activatePickMode(const QString& entityType) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& ctx = currentContext();
        ctx.m_pickModeActive = true;
        ctx.m_selectedType = entityType;
    }

    // Update SelectManager
    auto& select_manager = Render::SelectManager::instance();
    select_manager.setPickEnabled(true);

    // Set pick type
    Geometry::EntityType type = entityTypeFromString(entityType);
    Render::SelectManager::PickTypes pick_types = Render::SelectManager::PickTypes::None;

    switch(type) {
    case Geometry::EntityType::Vertex:
        pick_types = Render::SelectManager::PickTypes::Vertex;
        break;
    case Geometry::EntityType::Edge:
        pick_types = Render::SelectManager::PickTypes::Edge;
        break;
    case Geometry::EntityType::Face:
        pick_types = Render::SelectManager::PickTypes::Face;
        break;
    case Geometry::EntityType::Solid:
        pick_types = Render::SelectManager::PickTypes::Solid;
        break;
    case Geometry::EntityType::Part:
        pick_types = Render::SelectManager::PickTypes::Part;
        break;
    default:
        pick_types = Render::SelectManager::PickTypes::Face;
        break;
    }

    select_manager.setPickTypes(pick_types);

    emit pickModeActiveChanged();
    emit selectedTypeChanged();
    emit pickModeChanged(m_contextKey, true, entityType);

    LOG_TRACE("PickManager: activated pick mode for type {}", entityType.toStdString());
}

void PickManager::deactivatePickMode() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& ctx = currentContext();
        ctx.m_pickModeActive = false;
    }

    Render::SelectManager::instance().setPickEnabled(false);

    emit pickModeActiveChanged();
    emit pickModeChanged(m_contextKey, false, selectedType());

    LOG_TRACE("PickManager: deactivated pick mode");
}

void PickManager::addSelection(const QString& entityType, int entityUid) {
    const Geometry::EntityType type = entityTypeFromString(entityType);
    const Geometry::EntityUID uid = static_cast<Geometry::EntityUID>(entityUid);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& ctx = currentContext();
        Render::SelectManager::PickResult result{type, uid};

        // Check if already exists
        bool found = false;
        for(const auto& sel : ctx.m_selections) {
            if(sel == result) {
                found = true;
                break;
            }
        }

        if(!found) {
            ctx.m_selections.push_back(result);
        }
    }

    // Sync to SelectManager
    Render::SelectManager::instance().addSelection(uid, type);

    emit selectedEntitiesChanged();
    emit selectionChanged(m_contextKey, selectedEntities());

    LOG_TRACE("PickManager: added selection {}:{}", entityType.toStdString(), entityUid);
}

void PickManager::removeSelection(const QString& entityType, int entityUid) {
    const Geometry::EntityType type = entityTypeFromString(entityType);
    const Geometry::EntityUID uid = static_cast<Geometry::EntityUID>(entityUid);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& ctx = currentContext();
        Render::SelectManager::PickResult result{type, uid};

        ctx.m_selections.erase(std::remove_if(ctx.m_selections.begin(), ctx.m_selections.end(),
                                              [&result](const auto& sel) { return sel == result; }),
                               ctx.m_selections.end());
    }

    Render::SelectManager::instance().removeSelection(uid, type);

    emit selectedEntitiesChanged();
    emit selectionChanged(m_contextKey, selectedEntities());

    LOG_TRACE("PickManager: removed selection {}:{}", entityType.toStdString(), entityUid);
}

void PickManager::clearSelection() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& ctx = currentContext();
        ctx.m_selections.clear();
    }

    Render::SelectManager::instance().clearSelections();

    emit selectedEntitiesChanged();
    emit selectionChanged(m_contextKey, selectedEntities());

    LOG_TRACE("PickManager: cleared all selections");
}

bool PickManager::isSelected(const QString& entityType, int entityUid) const {
    const Geometry::EntityType type = entityTypeFromString(entityType);
    const Geometry::EntityUID uid = static_cast<Geometry::EntityUID>(entityUid);

    std::lock_guard<std::mutex> lock(m_mutex);
    const auto& ctx = currentContext();
    Render::SelectManager::PickResult result{type, uid};

    for(const auto& sel : ctx.m_selections) {
        if(sel == result) {
            return true;
        }
    }

    return false;
}

int PickManager::selectionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(currentContext().m_selections.size());
}

// =============================================================================
// Internal Methods
// =============================================================================

void PickManager::handleEntityPicked(Geometry::EntityType type, Geometry::EntityUID uid) {
    const QString type_str = entityTypeToString(type);
    const int uid_int = static_cast<int>(uid);

    // Check if we should expand Part/Solid to descendant faces
    bool should_expand = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        should_expand = m_expandPartSolidSelection;
    }

    if(should_expand &&
       (type == Geometry::EntityType::Solid || type == Geometry::EntityType::Part)) {
        // Expand to descendant faces (replaces current selection)
        expandToDescendantFaces(type, uid);
    } else {
        // Normal selection: add to existing selection
        addSelection(type_str, uid_int);
    }

    emit entityPicked(m_contextKey, type_str, uid_int);

    LOG_TRACE("PickManager: entity picked {}:{}", type_str.toStdString(), uid_int);
}

PickManager::SelectionContext& PickManager::currentContext() {
    const std::string key = m_contextKey.toStdString();
    auto it = m_contexts.find(key);
    if(it == m_contexts.end()) {
        m_contexts[key] = SelectionContext{};
        return m_contexts[key];
    }
    return it->second;
}

const PickManager::SelectionContext& PickManager::currentContext() const {
    const std::string key = m_contextKey.toStdString();
    auto it = m_contexts.find(key);
    if(it == m_contexts.end()) {
        static SelectionContext empty;
        return empty;
    }
    return it->second;
}

void PickManager::syncToSelectManager() {
    auto& select_manager = Render::SelectManager::instance();

    std::lock_guard<std::mutex> lock(m_mutex);
    const auto& ctx = currentContext();

    // Clear and re-add all selections
    select_manager.clearSelections();
    for(const auto& sel : ctx.m_selections) {
        select_manager.addSelection(sel.m_uid, sel.m_type);
    }

    select_manager.setPickEnabled(ctx.m_pickModeActive);
}

void PickManager::syncFromSelectManager() {
    auto& select_manager = Render::SelectManager::instance();
    auto selections = select_manager.selections();

    std::lock_guard<std::mutex> lock(m_mutex);
    auto& ctx = currentContext();
    ctx.m_selections = std::move(selections);
}

void PickManager::expandToDescendantFaces(Geometry::EntityType type, Geometry::EntityUID uid) {
    LOG_TRACE("PickManager: expanding {}:{} to descendant faces", static_cast<int>(type), uid);

    // Clear current selection first (Part/Solid selection replaces current selection)
    clearSelection();

    // Get the geometry document manager to access entity relationships
    try {
        auto doc_manager =
            g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>();
        if(!doc_manager) {
            LOG_WARN("PickManager: document manager not available");
            // Fallback: just add the entity itself
            addSelection(entityTypeToString(type), static_cast<int>(uid));
            return;
        }

        // Get current document
        auto document = doc_manager->currentDocument();
        if(!document) {
            LOG_WARN("PickManager: no current document");
            addSelection(entityTypeToString(type), static_cast<int>(uid));
            return;
        }

        // Get all descendant faces
        auto descendant_faces = document->getDescendantFaces(uid, type);

        if(descendant_faces.empty()) {
            // No descendant faces found, add the entity itself
            LOG_INFO("PickManager: Part/Solid {} has no descendant faces, adding entity", uid);
            addSelection(entityTypeToString(type), static_cast<int>(uid));
            return;
        }

        // Add all descendant faces to selection
        LOG_INFO("PickManager: expanding {}:{} to {} descendant faces", static_cast<int>(type), uid,
                 descendant_faces.size());

        auto& select_manager = Render::SelectManager::instance();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto& ctx = currentContext();

            for(const auto& [face_type, face_uid] : descendant_faces) {
                Render::SelectManager::PickResult result{face_type, face_uid};
                ctx.m_selections.push_back(result);
                select_manager.addSelection(face_uid, face_type);
            }
        }

        emit selectedEntitiesChanged();
        emit selectionChanged(m_contextKey, selectedEntities());

    } catch(const std::exception& e) {
        LOG_ERROR("PickManager: failed to expand selection - {}", e.what());
        addSelection(entityTypeToString(type), static_cast<int>(uid));
    }
}

Geometry::EntityType PickManager::entityTypeFromString(const QString& str) {
    if(str == "Vertex")
        return Geometry::EntityType::Vertex;
    if(str == "Edge")
        return Geometry::EntityType::Edge;
    if(str == "Face")
        return Geometry::EntityType::Face;
    if(str == "Solid")
        return Geometry::EntityType::Solid;
    if(str == "Part")
        return Geometry::EntityType::Part;
    return Geometry::EntityType::None;
}

QString PickManager::entityTypeToString(Geometry::EntityType type) {
    switch(type) {
    case Geometry::EntityType::Vertex:
        return "Vertex";
    case Geometry::EntityType::Edge:
        return "Edge";
    case Geometry::EntityType::Face:
        return "Face";
    case Geometry::EntityType::Solid:
        return "Solid";
    case Geometry::EntityType::Part:
        return "Part";
    default:
        return "Unknown";
    }
}

} // namespace OpenGeoLab::App
