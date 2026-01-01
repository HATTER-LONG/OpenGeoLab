/**
 * @file model_data.cpp
 * @brief QML-exposed model data implementation
 */
#include "app/model_data.hpp"
#include "util/logger.hpp"

#include <nlohmann/json.hpp>

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

void ModelPartData::updateFromGeometry(const IO::ModelPart& part,
                                       const IO::GeometryData& geometry) {
    m_id = part.id;
    m_name = QString::fromStdString(part.name);
    m_solidCount = part.solid_ids.size();

    // Count faces, edges, vertices in this part's solids
    int faces = 0, edges = 0, vertices = 0;
    for(uint32_t solid_id : part.solid_ids) {
        for(const auto& solid : geometry.solids) {
            if(solid.id == solid_id) {
                faces += solid.face_ids.size();
                break;
            }
        }
    }

    m_faceCount = faces;
    m_edgeCount = geometry.edges.size();
    m_vertexCount = geometry.vertices.size();

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

void ModelManager::loadFromResult(const QVariantMap& result) {
    clear();

    // Extract geometry data from result
    // For now, we store summary info in the result
    int partCount = result.value("parts", 0).toInt();
    int solidCount = result.value("solids", 0).toInt();
    int faceCount = result.value("faces", 0).toInt();
    int edgeCount = result.value("edges", 0).toInt();
    int vertexCount = result.value("vertices", 0).toInt();

    if(partCount == 0) {
        LOG_WARN("No parts in geometry data");
        emit hasModelChanged();
        return;
    }

    // Create sample part entry with statistics
    auto partData = new ModelPartData(this);
    partData->setData(1, QString::fromStdString("Part 1"), solidCount, faceCount, edgeCount,
                      vertexCount);

    m_parts.append(partData);

    LOG_INFO("Loaded model data: {} parts, {} solids, {} faces, {} edges, {} vertices", partCount,
             solidCount, faceCount, edgeCount, vertexCount);

    emit partsChanged();
    emit hasModelChanged();
}

void ModelManager::clear() {
    qDeleteAll(m_parts);
    m_parts.clear();
    emit partsChanged();
    emit hasModelChanged();
}

} // namespace OpenGeoLab::App
