/// @file generate_surface_mesh_action.cpp
/// @brief Generates a 2D surface mesh via Gmsh and stores it in MeshStore.

#include <opengeolab/mesh/generate_surface_mesh_action.hpp>

#include <opengeolab/mesh/gmsh_bridge.hpp>
#include <opengeolab/mesh/mesh_params.hpp>
#include <opengeolab/mesh/mesh_store.hpp>
#include <opengeolab/mesh/mesh_visual_builder.hpp>

#include <opengeolab/core/logger.hpp>
#include <opengeolab/geometry/geometry_module.hpp>

namespace OpenGeoLab::Mesh {

GenerateSurfaceMeshAction::GenerateSurfaceMeshAction(
    MeshStore& store, Kangaroo::Util::PluginComponentFactory& factory)
    : m_store(store), m_factory(factory) {}

GenerateSurfaceMeshAction::~GenerateSurfaceMeshAction() = default;

nlohmann::json GenerateSurfaceMeshAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Generate a 2D surface mesh on an OCC shape using Gmsh."},
        {"params",
         {{"shapeId",
           {{"type", "integer"}, {"required", true}, {"description", "Source shape ID"}}},
          {"name",
           {{"type", "string"},
            {"required", false},
            {"description", "Mesh name (auto-generated if omitted)"}}},
          {"minSize",
           {{"type", "number"},
            {"required", false},
            {"description", "Minimum element size (default 0.1)"}}},
          {"maxSize",
           {{"type", "number"},
            {"required", false},
            {"description", "Maximum element size (default 10.0)"}}},
          {"algorithm",
           {{"type", "integer"},
            {"required", false},
            {"description", "Mesh algorithm: 1=MeshAdapt, 5=Delaunay, 6=Frontal-Delaunay(default), "
                            "7=BAMG"}}},
          {"quadDominant",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Generate quad-dominant mesh (default false)"}}},
          {"order",
           {{"type", "integer"},
            {"required", false},
            {"description", "Element order: 1(default) or 2"}}},
          {"optimize",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Optimize mesh quality (default true)"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success"}}},
          {"meshId", {{"type", "integer"}, {"description", "Assigned mesh ID"}}},
          {"name", {{"type", "string"}, {"description", "Mesh name"}}},
          {"nodeCount", {{"type", "integer"}, {"description", "Number of mesh nodes"}}},
          {"elementCount", {{"type", "integer"}, {"description", "Number of elements"}}},
          {"elementTypes", {{"type", "array"}, {"description", "List of element type names"}}}}}};
}

nlohmann::json GenerateSurfaceMeshAction::execute(const nlohmann::json& param,
                                                  const Core::ProgressCallback& progress) {
    // 1. Validate shapeId
    if(!param.contains("shapeId") || !param["shapeId"].is_number_integer()) {
        return {{"ok", false}, {"summary", "Missing or invalid 'shapeId' parameter"}};
    }
    const auto shape_id = param["shapeId"].get<uint32_t>();

    // 2. Retrieve OCC shape from GeometryModule
    if(progress) {
        progress(0.0, "Retrieving shape...");
    }

    auto geo_module =
        m_factory.getSharedInstance<Core::ModuleBase>(Geometry::GeometryModule::MODULE_NAME);
    if(!geo_module) {
        return {{"ok", false}, {"summary", "GeometryModule not available"}};
    }
    auto& geo_mod = static_cast<Geometry::GeometryModule&>(*geo_module);
    const auto* shape_entry = geo_mod.shapeStore().find(shape_id);
    if(!shape_entry) {
        return {{"ok", false}, {"summary", "Shape ID " + std::to_string(shape_id) + " not found"}};
    }

    // 3. Build params from JSON
    SurfaceMeshParams mesh_params;
    mesh_params.minSize = param.value("minSize", mesh_params.minSize);
    mesh_params.maxSize = param.value("maxSize", mesh_params.maxSize);
    mesh_params.algorithm = param.value("algorithm", mesh_params.algorithm);
    mesh_params.quadDominant = param.value("quadDominant", mesh_params.quadDominant);
    mesh_params.order = param.value("order", mesh_params.order);
    mesh_params.optimize = param.value("optimize", mesh_params.optimize);

    // 4. Generate mesh
    if(progress) {
        progress(0.1, "Generating surface mesh...");
    }
    LOG_INFO("GenerateSurfaceMeshAction: shapeId={}, minSize={}, maxSize={}", shape_id,
             mesh_params.minSize, mesh_params.maxSize);

    MeshEntry entry;
    try {
        entry = GmshBridge::generateSurfaceMesh(shape_entry->shape, mesh_params, progress);
    } catch(const std::exception& ex) {
        LOG_ERROR("GenerateSurfaceMeshAction: Gmsh error: {}", ex.what());
        return {{"ok", false}, {"summary", std::string("Gmsh error: ") + ex.what()}};
    }

    // 5. Assign name and source shape
    auto name = param.value("name", std::string{});
    if(name.empty()) {
        name = "SurfaceMesh_" + std::to_string(shape_id);
    }
    entry.name = name;
    entry.sourceShapeId = shape_id;

    // 6. Build visual data
    if(progress) {
        progress(0.8, "Building render data...");
    }
    entry.visualData =
        std::make_shared<Core::VisualData>(MeshVisualBuilder::buildVisualData(entry));

    auto tags = MeshVisualBuilder::buildEntityTags(entry);
    entry.nodeTags = std::move(tags.nodeTags);
    entry.edgeTags = std::move(tags.edgeTags);
    entry.elementTags = std::move(tags.elementTags);

    // 7. Collect element type names before moving
    nlohmann::json element_types = nlohmann::json::array();
    for(const auto& block : entry.surfaceBlocks) {
        element_types.push_back(std::to_string(static_cast<int>(block.type)));
    }

    const auto node_count = entry.nodeCount();
    const auto element_count = entry.elementCount();

    // 8. Store in MeshStore
    const auto mesh_id = m_store.add(std::move(entry));

    if(progress) {
        progress(1.0, "Done");
    }

    LOG_INFO("GenerateSurfaceMeshAction: meshId={}, nodes={}, elements={}", mesh_id, node_count,
             element_count);

    return {{"ok", true},
            {"action", ACTION_NAME},
            {"meshId", mesh_id},
            {"name", name},
            {"nodeCount", node_count},
            {"elementCount", element_count},
            {"elementTypes", element_types}};
}

} // namespace OpenGeoLab::Mesh
