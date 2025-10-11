// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SQUIRCLE_H
#define SQUIRCLE_H

#include "geometry.h"
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <memory>


//! [1]
class SquircleRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
public:
    ~SquircleRenderer();

    void setT(qreal t) { m_t = t; }
    void setViewportSize(const QSize& size) { m_viewportSize = size; }
    void setViewportOffset(const QPoint& offset) { m_viewportOffset = offset; }
    void setWindow(QQuickWindow* window) { m_window = window; }

    // 设置当前几何体类型
    void setGeometryType(const QString& type) { m_geometryType = type; }

public slots:
    void init();
    void paint();

private:
    // 渲染不同的几何体
    void renderSquircle();
    void renderCube();

    QSize m_viewportSize;
    QPoint m_viewportOffset;
    qreal m_t = 0.0;
    qreal m_rotation = 0.0; // 旋转角度

    QString m_geometryType = "squircle"; // 当前几何体类型

    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLShaderProgram* m_cubeProgram = nullptr; // 立方体着色器
    QQuickWindow* m_window = nullptr;

    QOpenGLBuffer m_vbo;     // Squircle 的 VBO
    QOpenGLBuffer m_cubeVBO; // 立方体顶点缓冲
    QOpenGLBuffer m_cubeEBO; // 立方体索引缓冲

    std::unique_ptr<GeometryData> m_squircleData;
    std::unique_ptr<GeometryData> m_cubeData;
};
//! [1]

//! [2]
class Squircle : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(qreal t READ t WRITE setT NOTIFY tChanged)
    Q_PROPERTY(
        QString geometryType READ geometryType WRITE setGeometryType NOTIFY geometryTypeChanged)
    QML_ELEMENT

public:
    Squircle();

    qreal t() const { return m_t; }
    void setT(qreal t);

    QString geometryType() const { return m_geometryType; }
    void setGeometryType(const QString& type);

signals:
    void tChanged();
    void geometryTypeChanged();

public slots:
    void sync();
    void cleanup();

private slots:
    void handleWindowChanged(QQuickWindow* win);

private:
    void releaseResources() override;

    qreal m_t;
    QString m_geometryType = "squircle";
    SquircleRenderer* m_renderer;
};
//! [2]

#endif // SQUIRCLE_H
