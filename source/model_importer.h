#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

class ModelImporter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit ModelImporter(QObject* parent = nullptr);

    Q_INVOKABLE void importModel(const QUrl& model_url);
};