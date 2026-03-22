/**
 * @file coin_quick_item.hpp
 * @brief QQuickFramebufferObject integration for Coin3D 3D viewport.
 */

#pragma once

#include <opengeolab/render/scene_manager.hpp>

#include <QQuickFramebufferObject>

#include <memory>

namespace OpenGeoLab::App {

/**
 * @brief QML-visible 3D viewport that renders a Coin3D scene graph.
 *
 * Uses QQuickFramebufferObject to render into an FBO. Mouse events
 * drive NavigationController for orbit/pan/zoom. Emits navigationFinished
 * signal with camera state JSON for recording integration.
 */
class CoinQuickItem : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int displayMode READ displayMode WRITE setDisplayMode NOTIFY displayModeChanged)

public:
    explicit CoinQuickItem(QQuickItem* parent = nullptr);
    ~CoinQuickItem() override;

    [[nodiscard]] auto createRenderer() const -> QQuickFramebufferObject::Renderer* override;
    [[nodiscard]] auto displayMode() const -> int;
    void setDisplayMode(int mode);

    /// Fit camera to show all scene objects.
    Q_INVOKABLE void viewAll();

    /// Restore camera from a JSON string (for replay).
    Q_INVOKABLE void restoreCameraState(const QString& json);

    /// Get current camera state as JSON string.
    Q_INVOKABLE QString cameraStateJson() const;

    /// Request a re-render (call after external scene changes).
    Q_INVOKABLE void requestUpdate();

signals:
    void displayModeChanged();

    /// Emitted after mouse navigation completes (on mouse release or wheel).
    void navigationFinished(const QString& cameraStateJson);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    std::shared_ptr<OpenGeoLab::Render::SceneManager> scene_manager_;
    QPointF last_mouse_pos_;
    bool is_orbiting_ = false;
    bool is_panning_ = false;
};

} // namespace OpenGeoLab::App
