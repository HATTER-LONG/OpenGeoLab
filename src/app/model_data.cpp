/**
 * @file model_data.cpp
 * @brief QML-exposed model data implementation.
 */
#include "app/model_data.hpp"
#include "geometry/geometry_model.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::App {

// --- ModelPartData Implementation ---

ModelPartData::ModelPartData(QObject* parent) : QObject(parent) {}

void ModelPartData::setData(
    uint id, const QString& name, int solids, int faces, int edges, int vertices) {
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
    m_id = part.id;
    m_name = QString::fromStdString(part.name);
    m_solidCount = static_cast<int>(part.solid_ids.size());

    // Count faces in this part's solids
    int faces = 0;
    for(uint32_t solid_id : part.solid_ids) {
        const Geometry::Solid* solid = model.getSolidById(solid_id);
        if(solid) {
            faces += static_cast<int>(solid->face_ids.size());
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

ModelManager::ModelManager(QObject* parent) : QObject(parent) {}

QList<QObject*> ModelManager::parts() const { return m_parts; }

void ModelManager::refreshFromStore() {
    clear();

    auto model = Geometry::GeometryStore::instance().getModel();
    if(!model || model->isEmpty()) {
        LOG_WARN("No geometry data in GeometryStore");
        return;
    }

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
