/**
 * @file viewport_service.hpp
 * @brief QML-accessible singleton for viewport focus management
 *
 * ViewportService provides a Q_PROPERTY-based API for QML to:
 * - Track which viewport is active / has keyboard focus
 * - Receive pick results as QML signals
 * - Drive camera commands from QML
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantMap>

namespace OpenGeoLab::App {

/**
 * @brief QML singleton managing viewport focus and forwarding pick/highlight events.
 *
 * Registered as a QML singleton named "ViewportService" so QML pages can:
 *   - Read `ViewportService.activeViewportId`
 *   - Receive `pickResult(uid, entityType)` signals
 */
class ViewportService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString activeViewportId READ activeViewportId WRITE setActiveViewportId NOTIFY
                   activeViewportIdChanged)
    Q_PROPERTY(bool pickEnabled READ isPickEnabled NOTIFY pickEnabledChanged)

public:
    explicit ViewportService(QObject* parent = nullptr);
    ~ViewportService() override = default;

    /**
     * @brief Get the current factory instance for QML.
     */
    static ViewportService* create(QQmlEngine* engine, QJSEngine* script_engine);

    [[nodiscard]] QString activeViewportId() const;
    void setActiveViewportId(const QString& id);

    [[nodiscard]] bool isPickEnabled() const;

signals:
    void activeViewportIdChanged();
    void pickEnabledChanged();

    /**
     * @brief Emitted when an entity is picked by the user.
     * @param uid Entity UID (24-bit masked)
     * @param entityType Entity type integer value
     * @param action 0=Add, 1=Remove
     */
    void pickResult(int uid, int entityType, int action);

    /**
     * @brief Emitted when hover highlight changes.
     * @param uid Hovered entity UID
     * @param entityType Hovered entity type
     */
    void hoverChanged(int uid, int entityType);

public slots:
    /**
     * @brief Request camera to fit to scene bounds.
     */
    void fitToScene();

    /**
     * @brief Request a predefined camera view.
     * @param viewName One of: "front", "back", "top", "bottom", "left", "right"
     */
    void setStandardView(const QString& viewName);

    /**
     * @brief Reset the camera to default position.
     */
    void resetCamera();

    /**
     * @brief Forward a pick event from the viewport to the service.
     * Used internally by GLViewport; QML pages can also call this.
     */
    void notifyPick(int uid, int entityType, int action);

    /**
     * @brief Forward a hover event from the viewport to the service.
     */
    void notifyHover(int uid, int entityType);

    /**
     * @brief Set geometry visibility for a specific part.
     * @param partUid Part entity UID
     * @param visible true to show, false to hide
     */
    void setPartGeometryVisible(int partUid, bool visible);

    /**
     * @brief Set mesh visibility for a specific part.
     * @param partUid Part entity UID
     * @param visible true to show, false to hide
     */
    void setPartMeshVisible(int partUid, bool visible);

    /**
     * @brief Check if geometry is visible for a given part.
     * @param partUid Part entity UID
     * @return true if visible
     */
    Q_INVOKABLE bool isPartGeometryVisible(int partUid) const;

    /**
     * @brief Check if mesh is visible for a given part.
     * @param partUid Part entity UID
     * @return true if visible
     */
    Q_INVOKABLE bool isPartMeshVisible(int partUid) const;

private:
    QString m_activeViewportId;
};

} // namespace OpenGeoLab::App
