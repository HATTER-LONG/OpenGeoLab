#pragma once

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

// 纯粹的OpenGL渲染器类，与Qt解耦，只负责渲染逻辑
class TriangleRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit TriangleRenderer();
    ~TriangleRenderer();

    void setColor(const QString& color);
    void setAngle(qreal angle);
    void setViewportSize(const QSize& size);
    void setViewportPosition(const QPoint& pos);
    void setWindow(QQuickWindow* window);

public slots:
    void init();
    void paint();

private:
    void updateColorUniform();

    QSize m_viewportSize;
    QPoint m_viewportPos;
    QString m_color;
    qreal m_angle = 0.0;
    QOpenGLShaderProgram* m_program = nullptr;
    QQuickWindow* m_window = nullptr;
    QOpenGLBuffer m_vbo;
    QVector3D m_colorVec;
};

// Qt Quick Item包装类，处理与QML的交互
class TriangleItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
    QML_NAMED_ELEMENT(TriangleItem)

public:
    TriangleItem();

    QString color() const { return m_color; }
    void setColor(const QString& color);

    qreal angle() const { return m_angle; }
    void setAngle(qreal angle);

    int fps() const { return m_fps; }

signals:
    void colorChanged();
    void angleChanged();
    void fpsChanged();

public slots:
    void sync();
    void cleanup();

private slots:
    void handleWindowChanged(QQuickWindow* win);

private:
    void releaseResources() override;
    void updateFps();

    QString m_color = "red";
    qreal m_angle = 0.0;
    TriangleRenderer* m_renderer = nullptr;

    // FPS 计算相关
    int m_fps = 0;
    int m_frameCount = 0;
    qint64 m_lastFpsTime = 0;
};
