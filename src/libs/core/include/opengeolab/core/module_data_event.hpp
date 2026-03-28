/// @file module_data_event.hpp
/// @brief ModuleDataEvent — generic data-change event types for modules
#pragma once

#include <cstdint>

namespace OpenGeoLab::Core {

/// @brief Module data-change event types.
///
/// Used by ModuleBase::dataChanged signal. Each module picks the event type
/// that best describes the mutation that occurred.
enum class ModuleDataEvent : uint8_t {
    ItemAdded,          ///< A new item was created or imported.
    ItemRemoved,        ///< An existing item was deleted.
    ItemRenamed,        ///< An item's name was changed.
    ItemModified,       ///< Item data/attributes changed (geometry ops, booleans, healing, etc.).
    DerivedDataUpdated, ///< Derived data updated (tessellation, meshing, analysis results, etc.).
    BulkChanged,        ///< Bulk change (batch import, scripted batch ops, etc.).
    Reset               ///< Full clear or replacement of all module data.
};

} // namespace OpenGeoLab::Core
