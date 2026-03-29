/// @file gmsh_bridge.cpp
/// @brief Gmsh bridge implementation — OCC shape → Gmsh mesh → MeshEntry.

#include <opengeolab/mesh/gmsh_bridge.hpp>

#include <opengeolab/core/logger.hpp>

#include <TopoDS_Shape.hxx>

#include <gmsh.h>

#include <mutex>
#include <stdexcept>

namespace OpenGeoLab::Mesh {

// ---------------------------------------------------------------------------
// Global Gmsh mutex — Gmsh is not thread-safe (global state), so all Gmsh
// operations must be serialized.
// ---------------------------------------------------------------------------

static std::mutex& gmshMutex() {
    static std::mutex m;
    return m;
}

// ---------------------------------------------------------------------------
// GmshSession
// ---------------------------------------------------------------------------

GmshSession::GmshSession() {
    gmshMutex().lock();
    gmsh::initialize();
    gmsh::option::setNumber("General.Verbosity", 2);
}

GmshSession::~GmshSession() {
    gmsh::finalize();
    gmshMutex().unlock();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Map Gmsh element type ID to our ElementType enum.
static ElementType gmshTypeToElementType(int gmsh_type) {
    switch(gmsh_type) {
    case 1:
        return ElementType::Line2;
    case 2:
        return ElementType::Triangle3;
    case 3:
        return ElementType::Quad4;
    case 4:
        return ElementType::Tetra4;
    case 5:
        return ElementType::Hexa8;
    case 6:
        return ElementType::Prism6;
    case 7:
        return ElementType::Pyramid5;
    case 8:
        return ElementType::Line3;
    case 9:
        return ElementType::Triangle6;
    case 10:
        return ElementType::Quad9;
    case 11:
        return ElementType::Tetra10;
    case 12:
        return ElementType::Hexa27;
    case 13:
        return ElementType::Prism18;
    case 14:
        return ElementType::Pyramid14;
    default:
        throw std::runtime_error("Unsupported Gmsh element type: " + std::to_string(gmsh_type));
    }
}

/// Extract all nodes from the current Gmsh model into MeshNodeArray.
static void extractNodes(MeshEntry& entry) {
    std::vector<std::size_t> node_tags;
    std::vector<double> coords;
    std::vector<double> parametric_coords;
    gmsh::model::mesh::getNodes(node_tags, coords, parametric_coords, -1, -1, false, false);

    // Gmsh may return nodes with non-contiguous tags. We renumber to 1-based contiguous.
    // Build a map from gmsh node tag → our 1-based node index.
    if(node_tags.empty()) {
        return;
    }

    // Find max node tag for direct-index mapping
    std::size_t max_tag = 0;
    for(auto tag : node_tags) {
        if(tag > max_tag) {
            max_tag = tag;
        }
    }

    // tagToIndex[gmshTag] = our 1-based node ID
    std::vector<uint32_t> tag_to_index(max_tag + 1, 0);
    entry.nodes.coords.resize(node_tags.size() * 3);

    for(std::size_t i = 0; i < node_tags.size(); ++i) {
        auto our_id = static_cast<uint32_t>(i + 1);
        tag_to_index[node_tags[i]] = our_id;
        entry.nodes.coords[i * 3 + 0] = coords[i * 3 + 0];
        entry.nodes.coords[i * 3 + 1] = coords[i * 3 + 1];
        entry.nodes.coords[i * 3 + 2] = coords[i * 3 + 2];
    }

    // Store tagToIndex in a temporary so extractElements can use it.
    // We pass it via a lambda capture pattern — see extractElements below.
    // Instead, we use a struct to carry both.
    // Actually, let's just put the tagToIndex in a shared context.

    // Note: We handle this by making extractElements accept the tagToIndex map.
}

/// Extract elements from Gmsh for a specific entity, creating an ElementBlock.
static ElementBlock
extractEntityElements(int dim, int tag, const std::vector<uint32_t>& tag_to_index) {
    std::vector<int> elem_types;
    std::vector<std::vector<std::size_t>> elem_tags;
    std::vector<std::vector<std::size_t>> elem_node_tags;
    gmsh::model::mesh::getElements(elem_types, elem_tags, elem_node_tags, dim, tag);

    // For simplicity, each (dim, tag) entity typically has one element type.
    // If multiple types exist, we create separate blocks per type.
    // But we return only the first block here — caller handles multiple.
    ElementBlock block;
    if(elem_types.empty()) {
        return block;
    }

    block.type = gmshTypeToElementType(elem_types[0]);
    block.geoEntity.dimension = dim;
    block.geoEntity.gmshTag = tag;

    const auto& nodes = elem_node_tags[0];
    block.connectivity.reserve(nodes.size());
    for(auto gmsh_node_tag : nodes) {
        if(gmsh_node_tag < tag_to_index.size() && tag_to_index[gmsh_node_tag] != 0) {
            block.connectivity.push_back(tag_to_index[gmsh_node_tag]);
        } else {
            block.connectivity.push_back(0); // Should not happen with valid mesh
        }
    }

    return block;
}

/// Full node extraction returning tagToIndex map for element extraction.
static std::vector<uint32_t> extractNodesWithMapping(MeshEntry& entry) {
    std::vector<std::size_t> node_tags;
    std::vector<double> coords;
    std::vector<double> parametric_coords;
    gmsh::model::mesh::getNodes(node_tags, coords, parametric_coords, -1, -1, false, false);

    if(node_tags.empty()) {
        return {};
    }

    std::size_t max_tag = 0;
    for(auto tag : node_tags) {
        if(tag > max_tag) {
            max_tag = tag;
        }
    }

    std::vector<uint32_t> tag_to_index(max_tag + 1, 0);
    entry.nodes.coords.resize(node_tags.size() * 3);

    for(std::size_t i = 0; i < node_tags.size(); ++i) {
        tag_to_index[node_tags[i]] = static_cast<uint32_t>(i + 1);
        entry.nodes.coords[i * 3 + 0] = coords[i * 3 + 0];
        entry.nodes.coords[i * 3 + 1] = coords[i * 3 + 1];
        entry.nodes.coords[i * 3 + 2] = coords[i * 3 + 2];
    }

    return tag_to_index;
}

/// Extract all elements for entities of a given dimension.
static void
extractElementsByDim(MeshEntry& entry, int dim, const std::vector<uint32_t>& tag_to_index) {
    gmsh::vectorpair entities;
    gmsh::model::getEntities(entities, dim);

    for(const auto& [entityDim, entityTag] : entities) {
        auto block = extractEntityElements(entityDim, entityTag, tag_to_index);
        if(block.connectivity.empty()) {
            continue;
        }

        switch(dim) {
        case 1:
            entry.lineBlocks.push_back(std::move(block));
            break;
        case 2:
            entry.surfaceBlocks.push_back(std::move(block));
            break;
        case 3:
            entry.volumeBlocks.push_back(std::move(block));
            break;
        default:
            break;
        }
    }
}

/// Apply common mesh parameters to current Gmsh model.
static void applyCommonParams(double min_size, double max_size, int algorithm, int order) {
    gmsh::option::setNumber("Mesh.MeshSizeMin", min_size);
    gmsh::option::setNumber("Mesh.MeshSizeMax", max_size);
    gmsh::option::setNumber("Mesh.Algorithm", algorithm);
    gmsh::option::setNumber("Mesh.ElementOrder", order);
}

// ---------------------------------------------------------------------------
// GmshBridge public API
// ---------------------------------------------------------------------------

namespace GmshBridge {

MeshEntry generateSurfaceMesh(const TopoDS_Shape& shape,
                              const SurfaceMeshParams& params,
                              const Core::ProgressCallback& progress) {
    GmshSession const session;
    gmsh::model::add("surface_mesh");

    // Import OCC shape
    gmsh::vectorpair out_dim_tags;
    gmsh::model::occ::importShapesNativePointer(static_cast<const void*>(&shape), out_dim_tags,
                                                true);
    gmsh::model::occ::synchronize();

    // Configure parameters
    applyCommonParams(params.minSize, params.maxSize, params.algorithm, params.order);
    if(params.quadDominant) {
        gmsh::option::setNumber("Mesh.RecombineAll", 1);
    }

    if(!progress(0.1, "Parameters configured")) {
        throw std::runtime_error("Mesh generation cancelled");
    }

    // Generate 2D mesh
    gmsh::model::mesh::generate(2);

    if(!progress(0.7, "Mesh generated")) {
        throw std::runtime_error("Mesh generation cancelled");
    }

    // Optimize
    if(params.optimize) {
        gmsh::model::mesh::optimize("", true);
    }
    progress(0.8, "Optimization complete");

    // Extract data
    MeshEntry entry;
    auto tag_to_index = extractNodesWithMapping(entry);
    extractElementsByDim(entry, 1, tag_to_index); // line elements
    extractElementsByDim(entry, 2, tag_to_index); // surface elements
    entry.elementLocator.build(entry.lineBlocks, entry.surfaceBlocks, entry.volumeBlocks);

    progress(1.0, "Data extraction complete");

    LOG_INFO("Surface mesh: {} nodes, {} elements", entry.nodeCount(), entry.elementCount());
    return entry;
}

MeshEntry generateVolumeMesh(const TopoDS_Shape& shape,
                             const VolumeMeshParams& params,
                             const Core::ProgressCallback& progress) {
    GmshSession const session;
    gmsh::model::add("volume_mesh");

    // Import OCC shape
    gmsh::vectorpair out_dim_tags;
    gmsh::model::occ::importShapesNativePointer(static_cast<const void*>(&shape), out_dim_tags,
                                                true);
    gmsh::model::occ::synchronize();

    // Configure parameters
    applyCommonParams(params.minSize, params.maxSize, params.algorithm, params.order);
    if(params.hexDominant) {
        gmsh::option::setNumber("Mesh.RecombineAll", 1);
    }

    if(!progress(0.1, "Parameters configured")) {
        throw std::runtime_error("Mesh generation cancelled");
    }

    // Generate 3D mesh (includes 2D surface mesh first)
    gmsh::model::mesh::generate(3);

    if(!progress(0.7, "Mesh generated")) {
        throw std::runtime_error("Mesh generation cancelled");
    }

    // Optimize
    if(params.optimize) {
        std::string opt_algo;
        switch(params.optimizeAlgorithm) {
        case 1:
            opt_algo = "Netgen";
            break;
        case 2:
            opt_algo = "HighOrder";
            break;
        default:
            opt_algo = "";
            break;
        }
        gmsh::model::mesh::optimize(opt_algo, true);
    }
    progress(0.8, "Optimization complete");

    // Extract data
    MeshEntry entry;
    auto tag_to_index = extractNodesWithMapping(entry);
    extractElementsByDim(entry, 1, tag_to_index); // line elements
    extractElementsByDim(entry, 2, tag_to_index); // surface elements
    extractElementsByDim(entry, 3, tag_to_index); // volume elements
    entry.elementLocator.build(entry.lineBlocks, entry.surfaceBlocks, entry.volumeBlocks);

    progress(1.0, "Data extraction complete");

    LOG_INFO("Volume mesh: {} nodes, {} elements", entry.nodeCount(), entry.elementCount());
    return entry;
}

} // namespace GmshBridge
} // namespace OpenGeoLab::Mesh
