/**
 * @file document_service.cpp
 * @brief Implementation of DocumentService for QML document access
 */

#include "app/document_service.hpp"
#include "geometry/geometry_document.hpp"
#include "geometry/geometry_document_manager.hpp"
#include "geometry/geometry_types.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::App {

// =============================================================================
// PartListModel Implementation
// =============================================================================

PartListModel::PartListModel(QObject* parent) : QAbstractListModel(parent) {
    LOG_TRACE("PartListModel created");
}

int PartListModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_parts.size());
}

QVariant PartListModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_parts.size())) {
        return {};
    }

    const auto& part = m_parts[static_cast<size_t>(index.row())];
    switch(role) {
    case EntityIdRole:
        return QVariant::fromValue(part.m_entityId);
    case NameRole:
        return part.m_name;
    case VertexCountRole:
        return part.m_vertexCount;
    case EdgeCountRole:
        return part.m_edgeCount;
    case WireCountRole:
        return part.m_wireCount;
    case FaceCountRole:
        return part.m_faceCount;
    case ShellCountRole:
        return part.m_shellCount;
    case SolidCountRole:
        return part.m_solidCount;
    default:
        return {};
    }
}

QHash<int, QByteArray> PartListModel::roleNames() const {
    return {{EntityIdRole, "entityId"},       {NameRole, "name"},
            {VertexCountRole, "vertexCount"}, {EdgeCountRole, "edgeCount"},
            {WireCountRole, "wireCount"},     {FaceCountRole, "faceCount"},
            {ShellCountRole, "shellCount"},   {SolidCountRole, "solidCount"}};
}

void PartListModel::refresh() {
    LOG_DEBUG("PartListModel: Refreshing part list");

    beginResetModel();
    m_parts.clear();

    try {
        auto manager_factory =
            g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>();
        if(!manager_factory) {
            LOG_WARN("PartListModel: Document manager factory not available");
            endResetModel();
            return;
        }

        auto document = manager_factory->currentDocument();
        if(!document) {
            LOG_WARN("PartListModel: No current document");
            endResetModel();
            return;
        }

        // Get part summaries from the document (uses internal caching)
        auto summaries = document->getPartSummaries();

        for(const auto& summary : summaries) {
            PartInfo info;
            info.m_entityId = static_cast<qint64>(summary.m_entityId);
            info.m_name = QString::fromStdString(summary.m_name);
            info.m_vertexCount = static_cast<int>(summary.m_vertexCount);
            info.m_edgeCount = static_cast<int>(summary.m_edgeCount);
            info.m_wireCount = static_cast<int>(summary.m_wireCount);
            info.m_faceCount = static_cast<int>(summary.m_faceCount);
            info.m_shellCount = static_cast<int>(summary.m_shellCount);
            info.m_solidCount = static_cast<int>(summary.m_solidCount);
            m_parts.push_back(info);
        }

        LOG_DEBUG("PartListModel: Found {} parts", m_parts.size());

    } catch(const std::exception& e) {
        LOG_ERROR("PartListModel: Exception refreshing: {}", e.what());
    }

    endResetModel();
}

// =============================================================================
// DocumentService Implementation
// =============================================================================

DocumentService::DocumentService(QObject* parent) : QObject(parent) {
    m_partListModel = new PartListModel(this);
    LOG_TRACE("DocumentService created");
    subscribeToDocument();
}

DocumentService::~DocumentService() { LOG_TRACE("DocumentService destroyed"); }

int DocumentService::partCount() const { return m_partCount; }

int DocumentService::totalEntityCount() const { return m_totalEntityCount; }

PartListModel* DocumentService::partListModel() { return m_partListModel; }

void DocumentService::refresh() {
    updateCounts();
    m_partListModel->refresh();
    emit documentChanged();
}

void DocumentService::onDocumentChanged() {
    LOG_DEBUG("DocumentService: Document changed, refreshing");
    refresh();
}

void DocumentService::subscribeToDocument() {
    // Initial refresh
    refresh();

    // Subscribe would be done here if we had a persistent connection
    // For now, RenderService triggers refreshes via geometryChanged signal
}

void DocumentService::updateCounts() {
    try {
        auto manager_factory =
            g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>();
        if(!manager_factory) {
            m_partCount = 0;
            m_totalEntityCount = 0;
            return;
        }

        auto document = manager_factory->currentDocument();
        if(!document) {
            m_partCount = 0;
            m_totalEntityCount = 0;
            return;
        }

        // Use efficient entity count methods
        m_partCount = static_cast<int>(document->entityCountByType(Geometry::EntityType::Part));
        m_totalEntityCount = static_cast<int>(document->entityCount());

        LOG_DEBUG("DocumentService: Updated counts - parts={}, total={}", m_partCount,
                  m_totalEntityCount);

    } catch(const std::exception& e) {
        LOG_ERROR("DocumentService: Exception updating counts: {}", e.what());
        m_partCount = 0;
        m_totalEntityCount = 0;
    }
}

} // namespace OpenGeoLab::App
