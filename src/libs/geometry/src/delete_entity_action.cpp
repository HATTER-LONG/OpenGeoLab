/**
 * @file delete_entity_action.cpp
 * @brief DeleteEntityAction — removes sub-entities from shapes
 */

#include <opengeolab/geometry/delete_entity_action.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepAlgoAPI_Defeaturing.hxx>
#include <BRepTools_ReShape.hxx>
#include <BRep_Builder.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shape.hxx>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {

DeleteEntityAction::DeleteEntityAction(ShapeStore& store) : m_store(store) {}
DeleteEntityAction::~DeleteEntityAction() = default;

nlohmann::json DeleteEntityAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Delete sub-entities (faces, solids) from shapes. "
                        "Face removal uses defeaturing (BRepAlgoAPI_Defeaturing). "
                        "Solid removal rebuilds the parent compound without the target solids."},
        {"params",
         {{"entities",
           {{"type", "array"},
            {"required", true},
            {"description", "Array of {shapeId, type, localId}. "
                            "type must be 'GeoFace' or 'GeoSolid'. "
                            "localId is 1-based index into the sub-shape map."}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true if all deletions succeeded."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"results",
           {{"type", "array"},
            {"description", "Per-shape status: modified, removed, failed, or unsupported."}}}}}};
}

namespace {

struct EntityGroup {
    std::vector<uint32_t> faceLocalIds;
    std::vector<uint32_t> solidLocalIds;
    bool hasUnsupported{false};
};

nlohmann::json makeFailureResult(uint32_t shape_id, const std::string& error) {
    return {{"shapeId", shape_id}, {"status", "failed"}, {"error", error}};
}

nlohmann::json
defeatureFaces(ShapeStore& store, uint32_t shape_id, const std::vector<uint32_t>& face_local_ids) {
    const auto* entry = store.find(shape_id);
    if(entry == nullptr) {
        return makeFailureResult(shape_id, "Shape not found");
    }

    TopTools_ListOfShape faces_to_remove;
    for(const auto local_id : face_local_ids) {
        const auto face = store.subShape(shape_id, Core::EntityType::GeoFace, local_id);
        if(face.IsNull()) {
            return makeFailureResult(shape_id, "Invalid face localId: " + std::to_string(local_id));
        }
        faces_to_remove.Append(face);
    }

    BRepAlgoAPI_Defeaturing defeaturing;
    defeaturing.SetShape(entry->shape);
    defeaturing.AddFacesToRemove(faces_to_remove);
    defeaturing.SetRunParallel(true);
    defeaturing.SetToFillHistory(true);
    defeaturing.Build();

    TopoDS_Shape result_shape;
    if(defeaturing.IsDone()) {
        result_shape = defeaturing.Shape();
    }

    TopTools_IndexedMapOfShape result_faces;
    if(!result_shape.IsNull()) {
        TopExp::MapShapes(result_shape, TopAbs_FACE, result_faces);
    }

    if(result_shape.IsNull() || result_faces.Extent() >= entry->faceMap.Extent()) {
        Handle(BRepTools_ReShape) reshape = new BRepTools_ReShape();
        for(const auto local_id : face_local_ids) {
            const auto face = store.subShape(shape_id, Core::EntityType::GeoFace, local_id);
            if(face.IsNull()) {
                return makeFailureResult(shape_id,
                                         "Invalid face localId: " + std::to_string(local_id));
            }
            reshape->Remove(face);
        }
        result_shape = reshape->Apply(entry->shape);
    }

    if(result_shape.IsNull()) {
        return makeFailureResult(shape_id,
                                 "Defeaturing failed: could not remove the requested faces");
    }

    store.replaceShape(shape_id, result_shape);
    store.tessellate(shape_id);

    return {{"shapeId", shape_id},
            {"status", "modified"},
            {"removedFaces", static_cast<int>(face_local_ids.size())}};
}

nlohmann::json
removeSolids(ShapeStore& store, uint32_t shape_id, const std::vector<uint32_t>& solid_local_ids) {
    const auto* entry = store.find(shape_id);
    if(entry == nullptr) {
        return makeFailureResult(shape_id, "Shape not found");
    }

    const int total_solids = entry->solidMap.Extent();
    const auto num_to_remove = static_cast<int>(solid_local_ids.size());

    for(const auto local_id : solid_local_ids) {
        if(local_id < 1 || static_cast<int>(local_id) > total_solids) {
            return makeFailureResult(shape_id,
                                     "Invalid solid localId: " + std::to_string(local_id));
        }
    }

    if(num_to_remove >= total_solids) {
        store.remove(shape_id);
        return {{"shapeId", shape_id}, {"status", "removed"}, {"removedSolids", num_to_remove}};
    }

    TopTools_IndexedMapOfShape solids_to_remove;
    for(const auto local_id : solid_local_ids) {
        solids_to_remove.Add(entry->solidMap.FindKey(static_cast<int>(local_id)));
    }

    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);
    for(TopoDS_Iterator it(entry->shape); it.More(); it.Next()) {
        if(!solids_to_remove.Contains(it.Value())) {
            builder.Add(result, it.Value());
        }
    }

    store.replaceShape(shape_id, result);
    store.tessellate(shape_id);

    return {{"shapeId", shape_id}, {"status", "modified"}, {"removedSolids", num_to_remove}};
}

} // namespace

nlohmann::json DeleteEntityAction::execute(const nlohmann::json& param,
                                           const Core::ProgressCallback& progress) {
    if(!param.contains("entities") || !param["entities"].is_array() || param["entities"].empty()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "No entities specified"}};
    }

    std::unordered_map<uint32_t, EntityGroup> groups;
    for(const auto& entity_json : param["entities"]) {
        const auto shape_id = entity_json.value("shapeId", static_cast<uint32_t>(0));
        const auto type_str = entity_json.value("type", std::string{});
        const auto local_id = entity_json.value("localId", static_cast<uint32_t>(0));

        auto& group = groups[shape_id];
        const auto entity_type = Core::parseEntityType(type_str);
        if(!entity_type.has_value()) {
            group.hasUnsupported = true;
            continue;
        }

        switch(*entity_type) {
        case Core::EntityType::GeoFace:
            group.faceLocalIds.push_back(local_id);
            break;
        case Core::EntityType::GeoSolid:
            group.solidLocalIds.push_back(local_id);
            break;
        default:
            group.hasUnsupported = true;
            break;
        }
    }

    if(progress) {
        progress(0.0, "Deleting entities...");
    }

    nlohmann::json results = nlohmann::json::array();
    bool all_ok = true;
    int processed = 0;
    const auto total = static_cast<int>(groups.size());

    for(auto& [shape_id, group] : groups) {
        if(group.hasUnsupported) {
            results.push_back({{"shapeId", shape_id},
                               {"status", "unsupported"},
                               {"error", "Edge/Vertex deletion is not supported in v1"}});
            all_ok = false;
            ++processed;
            if(progress) {
                progress(static_cast<double>(processed) / static_cast<double>(total),
                         "Processing...");
            }
            continue;
        }

        if(m_store.find(shape_id) == nullptr) {
            results.push_back(
                {{"shapeId", shape_id}, {"status", "failed"}, {"error", "Unknown shapeId"}});
            all_ok = false;
            ++processed;
            if(progress) {
                progress(static_cast<double>(processed) / static_cast<double>(total),
                         "Processing...");
            }
            continue;
        }

        if(!group.faceLocalIds.empty()) {
            auto face_result = defeatureFaces(m_store, shape_id, group.faceLocalIds);
            if(face_result["status"] == "failed") {
                all_ok = false;
            }
            results.push_back(std::move(face_result));
        }

        if(!group.solidLocalIds.empty() && m_store.find(shape_id) != nullptr) {
            auto solid_result = removeSolids(m_store, shape_id, group.solidLocalIds);
            if(solid_result["status"] == "failed") {
                all_ok = false;
            }
            results.push_back(std::move(solid_result));
        }

        ++processed;
        if(progress) {
            progress(static_cast<double>(processed) / static_cast<double>(total), "Processing...");
        }
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", all_ok}, {"action", ACTION_NAME}, {"results", results}};
}

} // namespace OpenGeoLab::Geometry
