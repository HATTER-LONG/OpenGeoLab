/**
 * @file model_importer.hpp
 * @brief QML singleton for importing 3D model files
 *
 * Provides QML interface for loading 3D models from various file formats.
 * Delegates actual file reading to IO component system.
 */

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

namespace OpenGeoLab {
namespace UI {
class Geometry3D;

/**
 * @brief QML interface for importing 3D geometry files
 *
 * This singleton class provides QML bindings for the model import functionality.
 * It delegates actual file reading to the IO component system.
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

    /**
     * @brief Get the file filter string for supported formats
     * @return Filter string for file dialogs (e.g., "3D Models (*.brep *.step)")
     */
    Q_INVOKABLE QString getSupportedFormatsFilter() const;

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
    Geometry3D* m_targetRenderer = nullptr;
};

} // namespace UI
} // namespace OpenGeoLab
