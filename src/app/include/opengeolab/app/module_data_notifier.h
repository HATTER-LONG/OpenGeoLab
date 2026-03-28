/// @file module_data_notifier.h
/// @brief ModuleDataNotifier — bridges module data-change events to Qt signals
#pragma once

#include <kangaroo/util/signal.hpp>

#include <QObject>

#include <vector>

namespace OpenGeoLab::Command {
class CommandDispatcher;
}

namespace OpenGeoLab::App {

/// @brief Bridges Kangaroo module data-change events to Qt signals.
///
/// Subscribes to module events via CommandDispatcher::onModuleDataChanged()
/// and forwards them as Qt signals using QueuedConnection, ensuring they
/// arrive on the main thread.
///
/// @note Lifetime must exceed the CommandDispatcher it subscribes to.
///       ScopedConnection handles auto-disconnect on destruction.
class ModuleDataNotifier : public QObject {
    Q_OBJECT

public:
    /// @param dispatcher CommandDispatcher reference (caller owns lifetime)
    /// @param parent QObject parent
    explicit ModuleDataNotifier(Command::CommandDispatcher& dispatcher, QObject* parent = nullptr);
    ~ModuleDataNotifier() override;

Q_SIGNALS:
    /// @brief Emitted on main thread when geometry module data changes.
    void geometryDataChanged();

private:
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
};

} // namespace OpenGeoLab::App
