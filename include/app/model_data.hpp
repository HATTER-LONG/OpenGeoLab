/**
 * @file model_data.hpp
 * @brief QML-exposed model data for UI display.
 *
 * Provides QML bridge to access geometry data stored in Geometry module.
 * This layer only handles QML property binding; actual data is in GeometryStore.
 */
#pragma once

#include "geometry/geometry_model.hpp"

#include <QObject>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

/**
 * @brief QML wrapper for model part information.
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
     */
    void setData(uint id, const QString& name, int solids, int faces, int edges, int vertices);

    /**
     * @brief Update from geometry part data.
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
 */
class ModelManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QList<QObject*> parts READ parts NOTIFY partsChanged)
    Q_PROPERTY(bool hasModel READ hasModel NOTIFY hasModelChanged)

public:
    explicit ModelManager(QObject* parent = nullptr);

    [[nodiscard]] QList<QObject*> parts() const;
    [[nodiscard]] bool hasModel() const { return !m_parts.isEmpty(); }

    /**
     * @brief Refresh model data from GeometryStore.
     *
     * Call this after import completes to update QML bindings.
     */
    Q_INVOKABLE void refreshFromStore();

    /**
     * @brief Load geometry data from import result (legacy).
     *
     * Calls refreshFromStore() internally since data is now in GeometryStore.
     */
    Q_INVOKABLE void loadFromResult(const QVariantMap& result);

    /**
     * @brief Clear all model data.
     */
    Q_INVOKABLE void clear();

signals:
    void partsChanged();
    void hasModelChanged();

private:
    QList<QObject*> m_parts;
};

} // namespace OpenGeoLab::App
