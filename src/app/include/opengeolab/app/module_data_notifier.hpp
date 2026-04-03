/**
 * @file module_data_notifier.hpp
 * @brief ModuleDataNotifier — bridges module data-change events to Qt signals
 */
#pragma once

#include <kangaroo/util/signal.hpp>

#include <QObject>

#include <vector>

namespace OpenGeoLab::Command {
class CommandDispatcher;
} // namespace OpenGeoLab::Command

namespace OpenGeoLab::App {

/**
 * @brief Bridges Kangaroo module data-change events to Qt signals.
 *
 * Subscribes to module events via CommandDispatcher::onModuleDataChanged()
 * and forwards them as Qt signals using QueuedConnection, ensuring they
 * arrive on the main thread.
 *
 * @note Lifetime must exceed the CommandDispatcher it subscribes to.
 *       ScopedConnection handles auto-disconnect on destruction.
 */
class ModuleDataNotifier : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Subscribe to module data-change events and forward as Qt signals.
     * @param dispatcher CommandDispatcher reference (caller owns lifetime).
     * @param parent QObject parent.
     */
    explicit ModuleDataNotifier(Command::CommandDispatcher& dispatcher, QObject* parent = nullptr);
    ~ModuleDataNotifier() override;

Q_SIGNALS:
    /** @brief Emitted on main thread when geometry module data changes. */
    void geometryDataChanged();

    /** @brief Emitted on main thread when scene structure changes (nodes, selection, visibility).
     */
    void sceneDataChanged();

    /**
     * @brief Emitted on main thread when the viewport needs a repaint.
     *
     * Fires for all scene module events (structural and viewport-only).
     * SidebarPanel should NOT connect to this — use sceneDataChanged instead.
     */
    void viewportRefreshNeeded();

private:
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
};

} // namespace OpenGeoLab::App
