/**
 * @file pick_manager.hpp
 * @brief QML-exposed singleton for geometry entity picking and selection
 *
 * PickManager bridges the C++ SelectManager with QML, providing:
 * - Pick mode activation/deactivation with entity type filtering
 * - Selection management (add, remove, clear)
 * - Context-based selection isolation for different UI panels
 * - Parent chain expansion for Part/Solid selection (adds all descendant faces)
 * - Signals for QML binding updates
 */

#pragma once

#include "render/select_manager.hpp"
#include "util/signal.hpp"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace OpenGeoLab::App {

/**
 * @brief QML singleton for interactive geometry picking
 *
 * Wraps Render::SelectManager and exposes a QML-friendly API.
 * Supports multiple selection contexts for different UI panels.
 *
 * Selection behavior:
 * - Vertex/Edge/Face: Multi-select, adds to existing selection
 * - Solid/Part: Single-select, replaces selection with all descendant faces
 *
 * Usage in QML:
 * @code
 * PickManager.activatePickMode("Face")
 * PickManager.selectedEntities  // [{type: "Face", uid: 123}, ...]
 * @endcode
 */
class PickManager final : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON

    /// Currently selected entity type for picking
    Q_PROPERTY(QString selectedType READ selectedType NOTIFY selectedTypeChanged)

    /// Whether pick mode is active
    Q_PROPERTY(bool pickModeActive READ pickModeActive NOTIFY pickModeActiveChanged)

    /// List of selected entities as QVariantList of {type, uid} maps
    Q_PROPERTY(QVariantList selectedEntities READ selectedEntities NOTIFY selectedEntitiesChanged)

    /// Current context key for selection isolation
    Q_PROPERTY(QString contextKey READ contextKey WRITE setContextKey NOTIFY contextKeyChanged)

    /// Whether to expand Part/Solid selections to descendant faces
    Q_PROPERTY(bool expandPartSolidSelection READ expandPartSolidSelection WRITE
                   setExpandPartSolidSelection NOTIFY expandPartSolidSelectionChanged)

public:
    explicit PickManager(QObject* parent = nullptr);
    ~PickManager() override;

    // =========================================================================
    // Property Accessors
    // =========================================================================

    [[nodiscard]] QString selectedType() const;
    [[nodiscard]] bool pickModeActive() const;
    [[nodiscard]] QVariantList selectedEntities() const;
    [[nodiscard]] QString contextKey() const;
    void setContextKey(const QString& key);
    [[nodiscard]] bool expandPartSolidSelection() const;
    void setExpandPartSolidSelection(bool expand);

    // =========================================================================
    // Q_INVOKABLE Methods for QML
    // =========================================================================

    /**
     * @brief Activate pick mode for a specific entity type
     * @param entityType Type to pick: "Vertex", "Edge", "Face", "Solid", "Part"
     */
    Q_INVOKABLE void activatePickMode(const QString& entityType);

    /**
     * @brief Deactivate pick mode
     */
    Q_INVOKABLE void deactivatePickMode();

    /**
     * @brief Add an entity to the current selection
     * @param entityType Entity type string
     * @param entityUid Entity unique identifier
     */
    Q_INVOKABLE void addSelection(const QString& entityType, int entityUid);

    /**
     * @brief Remove an entity from the current selection
     * @param entityType Entity type string
     * @param entityUid Entity unique identifier
     */
    Q_INVOKABLE void removeSelection(const QString& entityType, int entityUid);

    /**
     * @brief Clear all selections in the current context
     */
    Q_INVOKABLE void clearSelection();

    /**
     * @brief Check if an entity is in the current selection
     * @param entityType Entity type string
     * @param entityUid Entity unique identifier
     * @return true if entity is selected
     */
    Q_INVOKABLE bool isSelected(const QString& entityType, int entityUid) const;

    /**
     * @brief Get the count of selected entities
     * @return Number of selected entities
     */
    Q_INVOKABLE int selectionCount() const;

    // =========================================================================
    // Internal Methods (called from viewport)
    // =========================================================================

    /**
     * @brief Handle entity picked from viewport
     * @param type Entity type
     * @param uid Entity unique identifier
     *
     * Called by GLViewport when user picks an entity.
     */
    void handleEntityPicked(Geometry::EntityType type, Geometry::EntityUID uid);

    /**
     * @brief Get singleton instance
     */
    static PickManager* instance();

signals:
    /// Emitted when selected type changes
    void selectedTypeChanged();

    /// Emitted when pick mode active state changes
    void pickModeActiveChanged();

    /// Emitted when selection list changes
    void selectedEntitiesChanged();

    /// Emitted when context key changes
    void contextKeyChanged();

    /// Emitted when expand Part/Solid selection setting changes
    void expandPartSolidSelectionChanged();

    /// Emitted when pick mode changes (for QML Connections)
    void pickModeChanged(const QString& contextKey, bool enabled, const QString& entityType);

    /// Emitted when selection changes (for QML Connections)
    void selectionChanged(const QString& contextKey, const QVariantList& entities);

    /// Emitted when an entity is picked (for QML Connections)
    void entityPicked(const QString& contextKey, const QString& entityType, int entityUid);

private:
    /**
     * @brief Selection context for a specific UI panel
     */
    struct SelectionContext {
        bool m_pickModeActive{false};
        QString m_selectedType;
        std::vector<Render::SelectManager::PickResult> m_selections;
    };

    [[nodiscard]] SelectionContext& currentContext();
    [[nodiscard]] const SelectionContext& currentContext() const;

    void syncToSelectManager();
    void syncFromSelectManager();

    /**
     * @brief Expand a Part/Solid selection to include all descendant faces
     * @param type Original entity type (Solid or Part)
     * @param uid Original entity UID
     *
     * When selecting a Solid or Part, this method finds all descendant Face
     * entities and replaces the current selection with those faces.
     */
    void expandToDescendantFaces(Geometry::EntityType type, Geometry::EntityUID uid);

    static Geometry::EntityType entityTypeFromString(const QString& str);
    static QString entityTypeToString(Geometry::EntityType type);

private:
    mutable std::mutex m_mutex;

    QString m_contextKey{"default"};
    std::unordered_map<std::string, SelectionContext> m_contexts;

    bool m_expandPartSolidSelection{true}; ///< Expand Part/Solid to descendant faces

    Util::ScopedConnection m_pickSettingsConn;
    Util::ScopedConnection m_selectionConn;

    static PickManager* s_instance;
};

} // namespace OpenGeoLab::App
