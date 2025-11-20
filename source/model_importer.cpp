#include "model_importer.h"
#include "logger.hpp"
#include <QFile>

ModelImporter::ModelImporter(QObject* parent) : QObject(parent) {}
void ModelImporter::importModel(const QUrl& model_url) {
    LOG_INFO("Importing model from URL: {}", model_url.path().toStdString());
}