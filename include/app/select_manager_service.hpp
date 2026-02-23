/**
 * @file select_manager_service.hpp
 * @brief QML-facing selection manager singleton service
 */

#pragma once

#include "util/signal.hpp"

#include <QObject>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

/**
 * @brief QML singleton that exposes selection state and commands
 *
 * This service is designed to be registered as a QML singleton
 * (via QML_ELEMENT + QML_SINGLETON) and provides a thin, UI-friendly API
 * for enabling/disabling selection mode and managing current selections.
 */
class SelectManagerService final : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON
public:
    explicit SelectManagerService(QObject* parent = nullptr);
    ~SelectManagerService() override;

    // =========================================================================
    // Selection Management
    // =========================================================================

    /**
     * @brief Enable selection mode with given select types bitmask
     * @param select_types Bitmask that encodes pick/selection types.
     */
    Q_INVOKABLE void activateSelectMode(uint32_t select_types);

    /**
     * @brief Disable selection mode
     */
    Q_INVOKABLE void deactivateSelectMode();

    /**
     * @brief Check whether selection mode is currently enabled
     * @return true if selection mode is enabled
     */
    Q_INVOKABLE bool isSelectEnabled() const;

    /**
     * @brief Clear all current selections
     */
    Q_INVOKABLE void clearSelection();

    /**
     * @brief Select an entity by its type-scoped uid and type name
     * @param entity_uid Type-scoped entity id
     * @param entity_type Entity type name (e.g. "Edge", "Face")
     */
    Q_INVOKABLE void selectEntity(uint64_t entity_uid, const QString& entity_type);

    /**
     * @brief Remove an entity from the current selection by uid and type
     * @param entity_uid Type-scoped entity id
     * @param entity_type Entity type name (e.g. "Edge", "Face")
     */
    Q_INVOKABLE void removeEntity(uint64_t entity_uid, const QString& entity_type);

    /**
     * @brief Check whether an entity (uid+type) is selected
     * @param entity_uid Type-scoped entity id
     * @param entity_type Entity type name
     * @return true if the entity is in the current selection set
     */
    Q_INVOKABLE bool isEntitySelected(uint64_t entity_uid, const QString& entity_type) const;

    /**
     * @brief Get all current selections
     * @return Vector of (uid, type) pairs
     */
    Q_INVOKABLE QVector<QPair<uint64_t, QString>> currentSelections() const;

signals:
    void selectModeChanged(uint32_t select_types);
    void entitySelected(uint64_t entity_uid, const QString& entity_type);
    void entityRemoved(uint64_t entity_uid, const QString& entity_type);
    void selectModeActivated(bool enabled);
    void selectionCleared();

private:
    Util::ScopedConnection m_selectSettingsConn;
    Util::ScopedConnection m_selectionConn;
    Util::ScopedConnection m_selectionEnable;
};
} // namespace OpenGeoLab::App