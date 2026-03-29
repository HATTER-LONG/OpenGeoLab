/// @file generate_volume_mesh_action.cpp
/// @brief Generates a 3D volume mesh via Gmsh and stores it in MeshStore.

#include <opengeolab/mesh/generate_volume_mesh_action.hpp>

#include <opengeolab/mesh/gmsh_bridge.hpp>
#include <opengeolab/mesh/mesh_params.hpp>
#include <opengeolab/mesh/mesh_store.hpp>
#include <opengeolab/mesh/mesh_visual_builder.hpp>

#include <opengeolab/core/logger.hpp>
#include <opengeolab/geometry/geometry_module.hpp>

namespace OpenGeoLab::Mesh {

GenerateVolumeMeshAction::GenerateVolumeMeshAction(MeshStore& store,
                                                   Kangaroo::Util::PluginComponentFactory& factory)
    : m_store(store), m_factory(factory) {}

GenerateVolumeMeshAction::~GenerateVolumeMeshAction() = default;

nlohmann::json GenerateVolumeMeshAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Generate a 3D volume mesh on an OCC shape using Gmsh."},
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
            {"description", "Volume algorithm: 1=Delaunay(default), 4=Frontal, 7=MMG3D, "
                            "10=HXT"}}},
          {"hexDominant",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Generate hex-dominant mesh (default false)"}}},
          {"order",
           {{"type", "integer"},
            {"required", false},
            {"description", "Element order: 1(default) or 2"}}},
          {"optimize",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Optimize mesh quality (default true)"}}},
          {"optimizeAlgorithm",
           {{"type", "integer"},
            {"required", false},
            {"description", "Optimize algorithm: 0=Default, 1=Netgen, 2=HighOrder"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success"}}},
          {"meshId", {{"type", "integer"}, {"description", "Assigned mesh ID"}}},
          {"name", {{"type", "string"}, {"description", "Mesh name"}}},
          {"nodeCount", {{"type", "integer"}, {"description", "Number of mesh nodes"}}},
          {"elementCount", {{"type", "integer"}, {"description", "Number of elements"}}},
          {"elementTypes", {{"type", "array"}, {"description", "List of element type names"}}}}}};
}

nlohmann::json GenerateVolumeMeshAction::execute(const nlohmann::json& param,
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
    VolumeMeshParams mesh_params;
    mesh_params.minSize = param.value("minSize", mesh_params.minSize);
    mesh_params.maxSize = param.value("maxSize", mesh_params.maxSize);
    mesh_params.algorithm = param.value("algorithm", mesh_params.algorithm);
    mesh_params.hexDominant = param.value("hexDominant", mesh_params.hexDominant);
    mesh_params.order = param.value("order", mesh_params.order);
    mesh_params.optimize = param.value("optimize", mesh_params.optimize);
    mesh_params.optimizeAlgorithm = param.value("optimizeAlgorithm", mesh_params.optimizeAlgorithm);

    // 4. Generate mesh
    if(progress) {
        progress(0.1, "Generating volume mesh...");
    }
    LOG_INFO("GenerateVolumeMeshAction: shapeId={}, minSize={}, maxSize={}", shape_id,
             mesh_params.minSize, mesh_params.maxSize);

    MeshEntry entry;
    try {
        entry = GmshBridge::generateVolumeMesh(shape_entry->shape, mesh_params, progress);
    } catch(const std::exception& ex) {
        LOG_ERROR("GenerateVolumeMeshAction: Gmsh error: {}", ex.what());
        return {{"ok", false}, {"summary", std::string("Gmsh error: ") + ex.what()}};
    }

    // 5. Assign name and source shape
    auto name = param.value("name", std::string{});
    if(name.empty()) {
        name = "VolumeMesh_" + std::to_string(shape_id);
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
    for(const auto& block : entry.volumeBlocks) {
        element_types.push_back(std::to_string(static_cast<int>(block.type)));
    }

    const auto node_count = entry.nodeCount();
    const auto element_count = entry.elementCount();

    // 8. Store in MeshStore
    const auto mesh_id = m_store.add(std::move(entry));

    if(progress) {
        progress(1.0, "Done");
    }

    LOG_INFO("GenerateVolumeMeshAction: meshId={}, nodes={}, elements={}", mesh_id, node_count,
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
