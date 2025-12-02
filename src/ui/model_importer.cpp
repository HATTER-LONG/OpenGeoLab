/**
 * @file model_importer.cpp
 * @brief Implementation of QML model importer interface
 *
 * Provides QML bindings for 3D model import functionality.
 * Delegates actual file reading to the IO component system.
 */

#include <model_importer.hpp>

#include <core/logger.hpp>
#include <geometry3d.hpp>
#include <io/model_reader_manager.hpp>

#include <QFileInfo>

namespace OpenGeoLab {
namespace UI {

ModelImporter::ModelImporter(QObject* parent) : QObject(parent) {
    // Initialize the IO component system
    IO::ModelReaderManager::instance().registerBuiltinReaders();
    LOG_INFO("ModelImporter initialized with IO component system");
}

void ModelImporter::setTargetRenderer(QObject* renderer) {
    m_targetRenderer = qobject_cast<Geometry3D*>(renderer);
    if(m_targetRenderer) {
        LOG_INFO("Target renderer set successfully");
    } else {
        LOG_DEBUG("Failed to set target renderer - invalid object type");
    }
}

void ModelImporter::importModel(const QUrl& file_url) {
    QString file_path = file_url.toLocalFile();
    LOG_INFO("Importing model from: {}", file_path.toStdString());

    if(!QFileInfo::exists(file_path)) {
        QString error = QString("File does not exist: %1").arg(file_path);
        LOG_ERROR(error.toStdString());
        emit modelLoadFailed(error);
        return;
    }

    // Use the IO component system to read the model
    std::string error_msg;
    auto geometry_data =
        IO::ModelReaderManager::instance().readModel(file_path.toStdString(), error_msg);

    if(!geometry_data) {
        QString error = QString::fromStdString(error_msg);
        if(error.isEmpty()) {
            error = "Failed to load geometry from file";
        }
        LOG_ERROR(error.toStdString());
        emit modelLoadFailed(error);
        return;
    }

    // Pass geometry data to renderer
    if(m_targetRenderer) {
        m_targetRenderer->setCustomGeometry(geometry_data);
        LOG_INFO("Model loaded successfully: {} vertices, {} indices", geometry_data->vertexCount(),
                 geometry_data->indexCount());
        emit modelLoaded(QFileInfo(file_path).fileName());
    } else {
        LOG_DEBUG("No target renderer set - model loaded but not displayed");
        emit modelLoadFailed("No renderer available");
    }
}

QString ModelImporter::getSupportedFormatsFilter() const {
    auto extensions = IO::ModelReaderManager::instance().getSupportedExtensions();

    QString filter = "3D Models (";
    for(size_t i = 0; i < extensions.size(); ++i) {
        if(i > 0) {
            filter += " ";
        }
        filter += "*" + QString::fromStdString(extensions[i]);
    }
    filter += ")";

    return filter;
}

} // namespace UI
} // namespace OpenGeoLab