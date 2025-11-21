/**
 * @file model_importer.h
 * @brief QML singleton for importing 3D model files
 *
 * Provides functionality to load 3D models from various file formats
 * (BREP, STEP) and convert them to renderable geometry data.
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUrl>
#include <memory>

namespace OpenGeoLab {
namespace Geometry {
class GeometryData;
} // namespace Geometry
namespace UI {
class Geometry3D;
} // namespace UI

namespace IO {

/**
 * @brief Model importer for loading 3D geometry files
 *
 * This singleton class handles importing 3D model files (BREP, STEP)
 * and converting them to renderable geometry data.
 */
class ModelImporter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ModelImporter(QObject* parent = nullptr);

    /**
     * @brief Set the target renderer for imported models
     * @param renderer QObject pointer to Geometry3D instance
     */
    Q_INVOKABLE void setTargetRenderer(QObject* renderer);

    /**
     * @brief Import a 3D model file
     * @param file_url URL of the model file to import
     */
    Q_INVOKABLE void importModel(const QUrl& file_url);

signals:
    /**
     * @brief Emitted when a model is successfully loaded
     * @param filename Name of the loaded file
     */
    void modelLoaded(const QString& filename);

    /**
     * @brief Emitted when model loading fails
     * @param error Error message describing the failure
     */
    void modelLoadFailed(const QString& error);

private:
    /**
     * @brief Load a BREP file and convert to geometry data
     * @param file_path Local file path to BREP file
     * @return Shared pointer to geometry data, or nullptr on failure
     */
    std::shared_ptr<Geometry::GeometryData> loadBrepFile(const QString& file_path);

    /**
     * @brief Load a STEP file and convert to geometry data
     * @param file_path Local file path to STEP file
     * @return Shared pointer to geometry data, or nullptr on failure
     */
    std::shared_ptr<Geometry::GeometryData> loadStepFile(const QString& file_path);

    UI::Geometry3D* m_targetRenderer = nullptr;
};

} // namespace IO
} // namespace OpenGeoLab