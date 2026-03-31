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
        dispatcher.onModuleDataChanged("scene", [this](Core::ModuleDataEvent /*event*/) {
            QMetaObject::invokeMethod(this, &ModuleDataNotifier::sceneDataChanged,
                                      Qt::QueuedConnection);
        });
    if(scene_handle.isConnected()) {
        m_connections.push_back(std::move(scene_handle));
    }
}

ModuleDataNotifier::~ModuleDataNotifier() = default;

} // namespace OpenGeoLab::App
