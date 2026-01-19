/**
 * @file geometry_document.hpp
 * @brief Document management for geometry entities
 *
 * Provides a container for managing all loaded geometry parts,
 * with support for selection, visibility control, and entity lookup.
 */

#pragma once

#include "geometry/geometry_entity.hpp"
#include "geometry/geometry_types.hpp"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {

class GeometryDocument;
using GeometryDocumentPtr = std::shared_ptr<GeometryDocument>;

/**
 * @brief Observer interface for document changes
 */
class IDocumentObserver {
public:
    virtual ~IDocumentObserver() = default;

    /**
     * @brief Called when a part is added to the document
     */
    virtual void onPartAdded(const std::shared_ptr<PartEntity>& part) = 0;

    /**
     * @brief Called when a part is removed from the document
     */
    virtual void onPartRemoved(EntityId partId) = 0;

    /**
     * @brief Called when the selection changes
     */
    virtual void onSelectionChanged(const std::vector<EntityId>& selectedIds) = 0;

    /**
     * @brief Called when entity visibility changes
     */
    virtual void onVisibilityChanged(EntityId entityId, bool visible) = 0;

    /**
     * @brief Called when the document is cleared
     */
    virtual void onDocumentCleared() = 0;
};

using IDocumentObserverPtr = std::shared_ptr<IDocumentObserver>;

/**
 * @brief Central document for managing geometry entities
 *
 * The GeometryDocument acts as the model in an MVC architecture,
 * holding all loaded geometry parts and managing selection state.
 */
class GeometryDocument : public std::enable_shared_from_this<GeometryDocument> {
public:
    GeometryDocument();
    ~GeometryDocument() = default;

    /**
     * @brief Get the singleton instance
     */
    static GeometryDocumentPtr instance();

    // ========================================================================
    // Part Management
    // ========================================================================

    /**
     * @brief Add a part to the document
     * @param part The part to add
     * @return true if successfully added
     */
    bool addPart(std::shared_ptr<PartEntity> part);

    /**
     * @brief Remove a part from the document
     * @param partId The ID of the part to remove
     * @return true if the part was found and removed
     */
    bool removePart(EntityId partId);

    /**
     * @brief Get a part by ID
     * @return The part, or nullptr if not found
     */
    [[nodiscard]] std::shared_ptr<PartEntity> findPart(EntityId partId) const;

    /**
     * @brief Get all parts in the document
     */
    [[nodiscard]] const std::vector<std::shared_ptr<PartEntity>>& parts() const { return m_parts; }

    /**
     * @brief Clear all parts from the document
     */
    void clear();

    /**
     * @brief Get the total number of parts
     */
    [[nodiscard]] size_t partCount() const { return m_parts.size(); }

    // ========================================================================
    // Entity Lookup
    // ========================================================================

    /**
     * @brief Find any entity by ID (searches all parts)
     */
    [[nodiscard]] GeometryEntityPtr findEntity(EntityId entityId) const;

    /**
     * @brief Find the part containing a given entity
     */
    [[nodiscard]] std::shared_ptr<PartEntity> findPartContaining(EntityId entityId) const;

    // ========================================================================
    // Selection Management
    // ========================================================================

    /**
     * @brief Get the current selection mode
     */
    [[nodiscard]] SelectionMode selectionMode() const { return m_selectionMode; }

    /**
     * @brief Set the selection mode
     */
    void setSelectionMode(SelectionMode mode);

    /**
     * @brief Select an entity
     * @param entityId The entity to select
     * @param addToSelection If true, add to current selection; otherwise replace
     */
    void select(EntityId entityId, bool addToSelection = false);

    /**
     * @brief Deselect an entity
     */
    void deselect(EntityId entityId);

    /**
     * @brief Clear the current selection
     */
    void clearSelection();

    /**
     * @brief Get the currently selected entity IDs
     */
    [[nodiscard]] const std::vector<EntityId>& selectedIds() const { return m_selectedIds; }

    /**
     * @brief Check if an entity is selected
     */
    [[nodiscard]] bool isSelected(EntityId entityId) const;

    /**
     * @brief Get the primary selected entity (first in selection)
     */
    [[nodiscard]] GeometryEntityPtr primarySelection() const;

    // ========================================================================
    // Visibility Management
    // ========================================================================

    /**
     * @brief Set the visibility of an entity
     */
    void setEntityVisible(EntityId entityId, bool visible);

    /**
     * @brief Show all entities
     */
    void showAll();

    /**
     * @brief Hide all entities
     */
    void hideAll();

    // ========================================================================
    // Bounding Box
    // ========================================================================

    /**
     * @brief Get the combined bounding box of all visible parts
     */
    [[nodiscard]] BoundingBox totalBoundingBox() const;

    // ========================================================================
    // Version Tracking
    // ========================================================================

    /**
     * @brief Get the current document version
     *
     * The version is incremented whenever the document changes (parts added/removed,
     * selection changed, etc.). Useful for detecting changes without observer pattern.
     */
    [[nodiscard]] uint64_t version() const { return m_version; }

    // ========================================================================
    // Observer Pattern
    // ========================================================================

    /**
     * @brief Add an observer to receive document change notifications
     */
    void addObserver(IDocumentObserverPtr observer);

    /**
     * @brief Remove an observer
     */
    void removeObserver(IDocumentObserverPtr observer);

private:
    void notifyPartAdded(const std::shared_ptr<PartEntity>& part);
    void notifyPartRemoved(EntityId partId);
    void notifySelectionChanged();
    void notifyVisibilityChanged(EntityId entityId, bool visible);
    void notifyDocumentCleared();

    void updateEntityIndex(const std::shared_ptr<PartEntity>& part);

private:
    std::vector<std::shared_ptr<PartEntity>> m_parts;
    std::unordered_map<EntityId, GeometryEntityWeakPtr> m_entityIndex;
    std::unordered_map<EntityId, EntityId> m_entityToPartMap;

    SelectionMode m_selectionMode{SelectionMode::Face};
    std::vector<EntityId> m_selectedIds;

    uint64_t m_version{0}; ///< Document version for change detection

    std::vector<IDocumentObserverPtr> m_observers;
};

} // namespace OpenGeoLab::Geometry
