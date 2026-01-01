/**
 * @file model_data.hpp
 * @brief QML-exposed model data for UI display.
 *
 * Provides QML bridge to access geometry data stored in Geometry module.
 * This layer only handles QML property binding; actual data is in GeometryStore.
 * Automatically refreshes when GeometryStore notifies of changes.
 */
#pragma once

#include "geometry/geometry_model.hpp"

#include <QObject>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

/**
 * @brief QML wrapper for model part information.
 *
 * Exposes part data (ID, name, entity counts) to QML for display in the model tree.
 */
class ModelPartData : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(uint id READ id NOTIFY idChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(int solidCount READ solidCount NOTIFY solidCountChanged)
    Q_PROPERTY(int faceCount READ faceCount NOTIFY faceCountChanged)
    Q_PROPERTY(int edgeCount READ edgeCount NOTIFY edgeCountChanged)
    Q_PROPERTY(int vertexCount READ vertexCount NOTIFY vertexCountChanged)

public:
    explicit ModelPartData(QObject* parent = nullptr);

    [[nodiscard]] uint id() const { return m_id; }
    [[nodiscard]] QString name() const { return m_name; }
    [[nodiscard]] int solidCount() const { return m_solidCount; }
    [[nodiscard]] int faceCount() const { return m_faceCount; }
    [[nodiscard]] int edgeCount() const { return m_edgeCount; }
    [[nodiscard]] int vertexCount() const { return m_vertexCount; }

    /**
     * @brief Set part data directly.
     * @param id Part identifier.
     * @param name Part display name.
     * @param solids Number of solids in part.
     * @param faces Number of faces in part.
     * @param edges Number of edges in part.
     * @param vertices Number of vertices in part.
     */
    void setData(uint id, const QString& name, int solids, int faces, int edges, int vertices);

    /**
     * @brief Update from geometry part data.
     * @param part Source part from GeometryModel.
     * @param model Parent model for entity count lookups.
     */
    void updateFromPart(const Geometry::Part& part, const Geometry::GeometryModel& model);

signals:
    void idChanged();
    void nameChanged();
    void solidCountChanged();
    void faceCountChanged();
    void edgeCountChanged();
    void vertexCountChanged();

private:
    uint m_id = 0;
    QString m_name;
    int m_solidCount = 0;
    int m_faceCount = 0;
    int m_edgeCount = 0;
    int m_vertexCount = 0;
};

/**
 * @brief QML-exposed model manager for displaying geometry hierarchy.
 *
 * Reads from GeometryStore and provides QML-bindable properties.
 * Automatically subscribes to GeometryStore change notifications
 * and refreshes QML bindings when geometry data changes.
 */
class ModelManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QList<QObject*> parts READ parts NOTIFY partsChanged)
    Q_PROPERTY(bool hasModel READ hasModel NOTIFY hasModelChanged)
    Q_PROPERTY(int totalSolids READ totalSolids NOTIFY modelStatsChanged)
    Q_PROPERTY(int totalFaces READ totalFaces NOTIFY modelStatsChanged)
    Q_PROPERTY(int totalEdges READ totalEdges NOTIFY modelStatsChanged)
    Q_PROPERTY(int totalVertices READ totalVertices NOTIFY modelStatsChanged)

public:
    explicit ModelManager(QObject* parent = nullptr);
    ~ModelManager() override;

    [[nodiscard]] QList<QObject*> parts() const;
    [[nodiscard]] bool hasModel() const { return !m_parts.isEmpty(); }
    [[nodiscard]] int totalSolids() const { return m_totalSolids; }
    [[nodiscard]] int totalFaces() const { return m_totalFaces; }
    [[nodiscard]] int totalEdges() const { return m_totalEdges; }
    [[nodiscard]] int totalVertices() const { return m_totalVertices; }

    /**
     * @brief Refresh model data from GeometryStore.
     *
     * Call this after import completes to update QML bindings.
     * Also called automatically when GeometryStore notifies of changes.
     */
    Q_INVOKABLE void refreshFromStore();

    /**
     * @brief Load geometry data from import result (legacy).
     *
     * Calls refreshFromStore() internally since data is now in GeometryStore.
     * @param result Import operation result (unused).
     */
    Q_INVOKABLE void loadFromResult(const QVariantMap& result);

    /**
     * @brief Clear all model data.
     */
    Q_INVOKABLE void clear();

signals:
    void partsChanged();
    void hasModelChanged();
    void modelStatsChanged();

    /**
     * @brief Emitted when geometry data has been updated.
     *
     * Connect to this signal to perform actions after geometry changes.
     */
    void geometryUpdated();

private:
    /**
     * @brief Internal handler for GeometryStore change notifications.
     */
    void onGeometryChanged();

    QList<QObject*> m_parts;
    int m_totalSolids = 0;
    int m_totalFaces = 0;
    int m_totalEdges = 0;
    int m_totalVertices = 0;

    size_t m_callbackId = 0; ///< Registered callback ID for GeometryStore notifications.
};

} // namespace OpenGeoLab::App
