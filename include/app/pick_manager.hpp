/**
 * @file pick_manager.hpp
 * @brief QML singleton for managing entity picking and selection
 *
 * PickManager coordinates entity picking between the viewport and QML UI.
 * It maintains selection state per context and handles pick mode activation.
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

/**
 * @brief QML singleton service for entity picking and selection management
 *
 * Provides:
 * - Pick mode activation with entity type filtering
 * - Selection storage per context key
 * - Single-pick mode for one-shot selections
 * - Integration with GLViewport for pick detection
 */
class PickManager final : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QObject* viewport READ viewport WRITE setViewport NOTIFY viewportChanged)

    Q_PROPERTY(QString activeConsumerKey READ activeConsumerKey WRITE setActiveConsumer NOTIFY
                   activeConsumerKeyChanged)
    Q_PROPERTY(QString contextKey READ contextKey WRITE setContext NOTIFY contextKeyChanged)

    Q_PROPERTY(QString selectedType READ selectedType NOTIFY selectedTypeChanged)
    Q_PROPERTY(bool pickModeActive READ pickModeActive NOTIFY pickModeActiveChanged)

    Q_PROPERTY(QVariantList selectedEntities READ selectedEntities NOTIFY selectedEntitiesChanged)

    Q_PROPERTY(bool autoAddToSelection READ autoAddToSelection WRITE setAutoAddToSelection NOTIFY
                   autoAddToSelectionChanged)

    Q_PROPERTY(bool singlePickActive READ singlePickActive NOTIFY singlePickActiveChanged)

public:
    explicit PickManager(QObject* parent = nullptr);
    ~PickManager() override;

    QObject* viewport() const;

    /**
     * @brief Set the viewport for pick detection
     * @param vp Pointer to GLViewport instance
     */
    Q_INVOKABLE void setViewport(QObject* vp);

    QString activeConsumerKey() const;

    /**
     * @brief Set the active consumer key for context switching
     * @param key Consumer identifier
     */
    Q_INVOKABLE void setActiveConsumer(const QString& key);

    QString contextKey() const;

    /**
     * @brief Set the context key for selection storage
     * @param key Context identifier
     */
    Q_INVOKABLE void setContext(const QString& key);

    Q_INVOKABLE void clearActiveConsumer();

    QString selectedType() const;
    bool pickModeActive() const;

    QVariantList selectedEntities() const;

    bool autoAddToSelection() const;
    void setAutoAddToSelection(bool enabled);

    bool singlePickActive() const;

    // Selection management

    /**
     * @brief Clear all selected entities
     */
    Q_INVOKABLE void clearSelection();

    /**
     * @brief Add an entity to the selection
     * @param entityType Entity type string (Face, Edge, Vertex, etc.)
     * @param entityUid Entity unique identifier
     */
    Q_INVOKABLE void addSelection(const QString& entityType, int entityUid);

    /**
     * @brief Remove an entity from the selection
     * @param entityType Entity type string
     * @param entityUid Entity unique identifier
     */
    Q_INVOKABLE void removeSelection(const QString& entityType, int entityUid);

    // Pick mode management

    /**
     * @brief Activate pick mode for a specific entity type
     * @param entityType Entity type to pick (Face, Edge, Vertex, Solid, Part)
     */
    Q_INVOKABLE void activatePickMode(const QString& entityType);

    /**
     * @brief Deactivate pick mode
     */
    Q_INVOKABLE void deactivatePickMode();

    /**
     * @brief Request single-pick mode for one-shot selection
     * @param entityType Entity type to pick
     * @param consumerKey Optional consumer key for context
     */
    Q_INVOKABLE void requestSinglePick(const QString& entityType, const QString& consumerKey = {});

signals:
    void viewportChanged();

    void activeConsumerKeyChanged();
    void contextKeyChanged();

    void selectedTypeChanged();
    void pickModeActiveChanged();

    void selectedEntitiesChanged();
    void autoAddToSelectionChanged();

    void singlePickActiveChanged();

    // Compatibility/bridge signals used by QML UI
    void pickModeChanged(const QString& contextKey, bool enabled, const QString& entityType);
    void selectionChanged(const QString& contextKey, const QVariant& entities);
    void entityPicked(const QString& contextKey, const QString& entityType, int entityUid);
    void pickCancelled(const QString& contextKey);

    // Single pick callbacks
    void singlePickStarted(const QString& contextKey, const QString& entityType);
    void singlePickFinished(const QString& contextKey, const QString& entityType, int entityUid);
    void singlePickCancelled(const QString& contextKey);

private slots:
    void onViewportEntityPicked(const QString& entityType, int entityUid);
    void onViewportEntityUnpicked(const QString& entityType, int entityUid);
    void onViewportPickCancelled();

private:
    QString effectiveContextKey() const;
    void ensureContext(const QString& key);
    void loadSelectionForContext(const QString& key);
    void saveSelectionForContext(const QString& key);

    void applyViewportState();
    void setPickModeInternal(bool enabled, const QString& entityType);
    void setSelectedEntitiesInternal(const QVariantList& entities);

    /**
     * @brief Sync selected entity UIDs to viewport for hover highlight exclusion
     */
    void syncSelectedUidsToViewport();

private:
    QPointer<QObject> m_viewport;

    QString m_activeConsumerKey;
    QString m_contextKey;

    QString m_selectedType;
    bool m_pickModeActive{false};

    QVariantList m_selectedEntities;
    bool m_autoAddToSelection{true};

    // per-context selection store
    QHash<QString, QVariantList> m_selectionByContext;

    // single pick
    bool m_singlePickActive{false};
    bool m_restoreAutoAddAfterSinglePick{true};

    QMetaObject::Connection m_connPicked;
    QMetaObject::Connection m_connCancelled;
};

} // namespace OpenGeoLab::App
