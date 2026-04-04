/**
 * @file geometry_module.cpp
 * @brief GeometryModule — registers geometry actions into the factory
 */

#include <opengeolab/geometry/create_box_action.hpp>
#include <opengeolab/geometry/create_cylinder_action.hpp>
#include <opengeolab/geometry/create_sphere_action.hpp>
#include <opengeolab/geometry/create_torus_action.hpp>
#include <opengeolab/geometry/delete_shape_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/geometry/import_brep_action.hpp>
#include <opengeolab/geometry/import_step_action.hpp>
#include <opengeolab/geometry/list_shapes_action.hpp>
#include <opengeolab/geometry/list_sub_shapes_action.hpp>
#include <opengeolab/geometry/query_shape_action.hpp>
#include <opengeolab/geometry/tessellate_action.hpp>

#include <opengeolab/core/module_data_event.hpp>

#include <functional>

namespace OpenGeoLab::Geometry {

GeometryModule::GeometryModule(Kangaroo::Util::PluginComponentFactory& factory)
    : ModuleBase(MODULE_NAME, "Geometry creation and manipulation module.", factory) {
    registerAction<CreateBoxAction>(std::ref(m_shapeStore));
    registerAction<CreateCylinderAction>(std::ref(m_shapeStore));
    registerAction<CreateSphereAction>(std::ref(m_shapeStore));
    registerAction<CreateTorusAction>(std::ref(m_shapeStore));
    registerAction<ImportBrepAction>(std::ref(m_shapeStore));
    registerAction<ImportStepAction>(std::ref(m_shapeStore));
    registerAction<TessellateAction>(std::ref(m_shapeStore));
    registerAction<QueryShapeAction>(std::ref(m_shapeStore));
    registerAction<ListShapesAction>(std::ref(m_shapeStore));
    registerAction<ListSubShapesAction>(std::ref(m_shapeStore));
    registerAction<DeleteShapeAction>(std::ref(m_shapeStore));

    // Bridge ShapeStore signals → ModuleBase::dataChanged for event bus
    m_storeConnections.push_back(
        m_shapeStore.shapeAdded.connect([this](uint32_t, const ShapeEntry&) {
            dataChanged.emit(Core::ModuleDataEvent::ItemAdded);
        }));
    m_storeConnections.push_back(m_shapeStore.shapeRemoved.connect(
        [this](uint32_t) { dataChanged.emit(Core::ModuleDataEvent::ItemRemoved); }));
    m_storeConnections.push_back(
        m_shapeStore.shapeUpdated.connect([this](uint32_t, const ShapeEntry&) {
            dataChanged.emit(Core::ModuleDataEvent::ItemModified);
        }));
    m_storeConnections.push_back(m_shapeStore.storeCleared.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::Reset); }));
}

GeometryModule::~GeometryModule() = default;

ShapeStore& GeometryModule::shapeStore() { return m_shapeStore; }
const ShapeStore& GeometryModule::shapeStore() const { return m_shapeStore; }

void GeometryModule::clearAll() { m_shapeStore.clear(); }

} // namespace OpenGeoLab::Geometry
