/// @file module_data_notifier.cpp
/// @brief ModuleDataNotifier implementation
#include "opengeolab/app/module_data_notifier.h"

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

    auto mesh_handle =
        dispatcher.onModuleDataChanged("mesh", [this](Core::ModuleDataEvent /*event*/) {
            QMetaObject::invokeMethod(this, &ModuleDataNotifier::meshDataChanged,
                                      Qt::QueuedConnection);
        });

    if(mesh_handle.isConnected()) {
        m_connections.push_back(std::move(mesh_handle));
    }
}

ModuleDataNotifier::~ModuleDataNotifier() = default;

} // namespace OpenGeoLab::App
