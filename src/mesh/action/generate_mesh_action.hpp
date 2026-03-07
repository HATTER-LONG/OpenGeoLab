/**
 * @file generate_mesh_action.hpp
 * @brief Mesh action for generating a mesh from geometry entities via Gmsh
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "mesh/mesh_action.hpp"
#include "mesh/mesh_element.hpp"
#include "mesh/mesh_node.hpp"
#include "mesh/mesh_types.hpp"

#include <TopoDS_Compound.hxx>

#include <string>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Mesh {

/// @brief Consolidates all data flowing through the Gmsh mesh generation pipeline.
struct GmshMeshContext {
    // --- Input (set by caller) ---
    double m_elementSize{1.0};
    double m_elementSizeMin{1.0};
    double m_elementSizeMax{2.0};
    int m_meshDimension{2};
    std::string m_elementType{"triangle"};
    std::string m_algorithm2D{"frontal"};
    std::string m_algorithm3D{"delaunay"};
    int m_elementOrder{1};
    bool m_optimizeMesh{true};

    // --- Shape data (set by collectFaceShapes) ---
    TopoDS_Compound m_compound;
    std::vector<uint64_t> m_facePartUids; ///< Part UID per face in compound (parallel array)

    // --- Gmsh mapping (set by importAndMapShape) ---
    std::unordered_map<int64_t, uint64_t> m_tagToPartUid; ///< dimTagKey(dim,tag) -> Part UID

    // --- Node mapping (set by extractNodes) ---
    std::unordered_map<size_t, MeshNodeId> m_gmshToLocal; ///< Gmsh node tag -> local MeshNodeId

    // --- Staged mesh data (committed only after successful pipeline completion) ---
    std::vector<MeshNode> m_nodes;
    std::vector<MeshElement> m_elements;
};

/**
 * @brief Action for generating mesh entities from selected geometry entities
 *
 * Request parameters:
 * - action: "generate_mesh"
 * - entities: array of objects {"uid": <number>, "type": <string>}
 * - elementSize: number (global mesh size)
 * - elementSizeMin: number (optional minimum mesh size)
 * - elementSizeMax: number (optional maximum mesh size)
 * - meshDimension: number (2 or 3, default 2)
 * - elementType: string ("triangle", "quad", "auto", default "triangle")
 * - algorithm2D: string ("automatic", "meshadapt", "delaunay", "frontal", "bamg",
 *   "frontal_quad")
 * - algorithm3D: string ("delaunay", "frontal", "mmg3d", "rtree", "hxt")
 * - elementOrder: number (1 or 2)
 * - optimizeMesh: boolean
 * - replaceExisting: boolean
 */
class GenerateMeshAction final : public MeshActionBase {
public:
    GenerateMeshAction() = default;
    ~GenerateMeshAction() override = default;

    [[nodiscard]] static std::string actionName() { return "generate_mesh"; }

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;

private:
    /**
     * @brief Collect Face shapes from selected entities and build OCC compound.
     *
     * For Part/Solid entities, all child faces inherit the Part's UID.
     * For Face entities, the parent Part's UID is looked up via relation query.
     * Fills ctx.m_compound and ctx.m_facePartUids.
     */
    void collectFaceShapes(const Geometry::EntityRefSet& entities, GmshMeshContext& ctx);

    /**
     * @brief Run the full Gmsh pipeline: import shape, generate mesh, extract to MeshDocument.
     *
     * Gmsh must already be initialized before calling this method.
     */
    [[nodiscard]] bool runGmshPipeline(GmshMeshContext& ctx,
                                       Util::ProgressCallback progress_callback);
};

class GenerateMeshActionFactory : public MeshActionFactory {
public:
    GenerateMeshActionFactory() = default;
    ~GenerateMeshActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<GenerateMeshAction>(); }
};

} // namespace OpenGeoLab::Mesh
