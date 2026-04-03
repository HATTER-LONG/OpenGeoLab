/**
 * @file module_data_notifier.cpp
 * @brief ModuleDataNotifier implementation
 */
#include "opengeolab/app/module_data_notifier.hpp"

#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/core/module_data_event.hpp>

namespace OpenGeoLab::App {

ModuleDataNotifier::ModuleDataNotifier(Command::CommandDispatcher& dispatcher, QObject* parent)
    : QObject(parent) {
    auto handle =
        dispatcher.onModuleDataChanged("geometry", [this](Core::ModuleDataEvent /*event*/) {
            QMetaObject::invokeMethod(this, &ModuleDataNotifier::geometryDataChanged,
                                      Qt::QueuedConnection);
        });

    if(handle.isConnected()) {
        m_connections.push_back(std::move(handle));
    }

    auto scene_handle =
        dispatcher.onModuleDataChanged("scene", [this](Core::ModuleDataEvent event) {
            // Viewport-only changes (camera, pick area) skip scene data refresh.
            if(event != Core::ModuleDataEvent::ViewportChanged) {
                QMetaObject::invokeMethod(this, &ModuleDataNotifier::sceneDataChanged,
                                          Qt::QueuedConnection);
            }
            // All events (including viewport-only) trigger viewport repaint.
            QMetaObject::invokeMethod(this, &ModuleDataNotifier::viewportRefreshNeeded,
                                      Qt::QueuedConnection);
        });
    if(scene_handle.isConnected()) {
        m_connections.push_back(std::move(scene_handle));
    }

    auto mesh_handle =
        dispatcher.onModuleDataChanged("mesh", [this](Core::ModuleDataEvent /*event*/) {
            QMetaObject::invokeMethod(this, &ModuleDataNotifier::sceneDataChanged,
                                      Qt::QueuedConnection);
            QMetaObject::invokeMethod(this, &ModuleDataNotifier::viewportRefreshNeeded,
                                      Qt::QueuedConnection);
        });
    if(mesh_handle.isConnected()) {
        m_connections.push_back(std::move(mesh_handle));
    }
}

ModuleDataNotifier::~ModuleDataNotifier() = default;

} // namespace OpenGeoLab::App
