/**
 * @file model_data.cpp
 * @brief QML-exposed model data implementation.
 *
 * Implements the bridge between GeometryStore and QML UI.
 * Subscribes to GeometryStore change notifications for automatic updates.
 */
#include "app/model_data.hpp"
#include "geometry/geometry_model.hpp"
#include "util/logger.hpp"

#include <QMetaObject>

namespace OpenGeoLab::App {

// --- ModelPartData Implementation ---

ModelPartData::ModelPartData(QObject* parent) : QObject(parent) {}

void ModelPartData::setData( // NOLINT
    uint id,
    const QString& name,
    int solids,
    int faces,
    int edges,
    int vertices) {
    m_id = id;
    m_name = name;
    m_solidCount = solids;
    m_faceCount = faces;
    m_edgeCount = edges;
    m_vertexCount = vertices;

    emit idChanged();
    emit nameChanged();
    emit solidCountChanged();
    emit faceCountChanged();
    emit edgeCountChanged();
    emit vertexCountChanged();
}

void ModelPartData::updateFromPart(const Geometry::Part& part,
                                   const Geometry::GeometryModel& model) {
    m_id = part.m_id;
    m_name = QString::fromStdString(part.m_name);
    m_solidCount = static_cast<int>(part.m_solidIds.size());

    // Count faces in this part's solids
    int faces = 0;
    for(uint32_t solid_id : part.m_solidIds) {
        const Geometry::Solid* solid = model.getSolidById(solid_id);
        if(solid) {
            faces += static_cast<int>(solid->m_faceIds.size());
        }
    }

    m_faceCount = faces;
    m_edgeCount = static_cast<int>(model.edgeCount());
    m_vertexCount = static_cast<int>(model.vertexCount());

    emit idChanged();
    emit nameChanged();
    emit solidCountChanged();
    emit faceCountChanged();
    emit edgeCountChanged();
    emit vertexCountChanged();
}

// --- ModelManager Implementation ---

ModelManager::ModelManager(QObject* parent) : QObject(parent) {
    // Register callback for geometry change notifications
    m_callbackId = Geometry::GeometryStore::instance().registerChangeCallback([this]() {
        // Use Qt's thread-safe mechanism to invoke on the main thread
        QMetaObject::invokeMethod(this, &ModelManager::onGeometryChanged, Qt::QueuedConnection);
    });

    LOG_INFO("ModelManager initialized with GeometryStore callback ID: {}", m_callbackId);
}

ModelManager::~ModelManager() {
    // Unregister callback when destroyed
    if(m_callbackId != 0) {
        Geometry::GeometryStore::instance().unregisterChangeCallback(m_callbackId);
        LOG_INFO("ModelManager unregistered GeometryStore callback ID: {}", m_callbackId);
    }
}

QList<QObject*> ModelManager::parts() const { return m_parts; }

void ModelManager::onGeometryChanged() {
    LOG_DEBUG("ModelManager received geometry change notification");
    refreshFromStore();
    emit geometryUpdated();
}

void ModelManager::refreshFromStore() {
    clear();

    auto model = Geometry::GeometryStore::instance().getModel();
    if(!model || model->isEmpty()) {
        LOG_WARN("No geometry data in GeometryStore");
        m_totalSolids = 0;
        m_totalFaces = 0;
        m_totalEdges = 0;
        m_totalVertices = 0;
        emit modelStatsChanged();
        return;
    }

    // Update total statistics
    m_totalSolids = static_cast<int>(model->solidCount());
    m_totalFaces = static_cast<int>(model->faceCount());
    m_totalEdges = static_cast<int>(model->edgeCount());
    m_totalVertices = static_cast<int>(model->vertexCount());

    const auto& model_parts = model->getParts();
    for(const auto& part : model_parts) {
        auto* part_data = new ModelPartData(this);
        part_data->updateFromPart(part, *model);
        m_parts.append(part_data);
    }

    LOG_INFO("Refreshed model data from GeometryStore: {} parts, {} solids, {} faces",
             model->partCount(), model->solidCount(), model->faceCount());

    emit partsChanged();
    emit hasModelChanged();
    emit modelStatsChanged();
}

void ModelManager::loadFromResult(const QVariantMap& /* result */) {
    // Data is now stored in GeometryStore by IO layer
    refreshFromStore();
}

void ModelManager::clear() {
    qDeleteAll(m_parts);
    m_parts.clear();
    emit partsChanged();
    emit hasModelChanged();
}

} // namespace OpenGeoLab::App
