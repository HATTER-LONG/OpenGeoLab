# Geometry Delete Feature — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add sub-entity deletion (face/solid) via a new `delete_entity` action, expose it through a DeleteEntityPage in the ribbon, and add right-click "Delete Shape" to the sidebar.

**Architecture:** New `DeleteEntityAction` in the geometry module uses `BRepAlgoAPI_Defeaturing` (face removal) and `BRep_Builder` compound rebuild (solid removal). A new `ShapeStore::replaceShape()` method enables in-place shape modification with signal-based scene synchronization. QML adds a DeleteEntityPage with picker workflow and a sidebar context menu for whole-shape deletion.

**Tech Stack:** C++20, OpenCASCADE (TKBO for Defeaturing), Qt6/QML, Doctest, Ninja/CMake

**Spec:** `docs/superpowers/specs/2026-04-10-geometry-delete-design.md`

---

## File Map

### New Files

| File | Responsibility |
|------|---------------|
| `src/libs/geometry/include/opengeolab/geometry/delete_entity_action.hpp` | Header for DeleteEntityAction |
| `src/libs/geometry/src/delete_entity_action.cpp` | Implementation: face defeaturing + solid removal |
| `src/libs/geometry/test/delete_entity_action_test.cpp` | Unit tests for delete_entity |
| `src/app/resource/qml/components/pages/DeleteEntityPage.qml` | QML function page with picker workflow |

### Modified Files

| File | Change |
|------|--------|
| `src/libs/geometry/include/opengeolab/geometry/shape_store.hpp` | Add `replaceShape()` declaration |
| `src/libs/geometry/src/shape_store.cpp` | Implement `replaceShape()` |
| `src/libs/geometry/CMakeLists.txt` | Add TKBO link, new source + header + test files |
| `src/libs/geometry/src/geometry_module.cpp` | Register DeleteEntityAction |
| `src/libs/geometry/test/geometry_module_test.cpp` | Update action count in describe test |
| `src/app/resource/qml/RibbonConfig.qml` | Add Delete to Modify group |
| `src/app/resource/qml/MainPages.qml` | Register DeleteEntityPage |
| `src/app/resource/qml/components/ShapeListItem.qml` | Add right-click context menu |
| `src/app/CMakeLists.txt` | Register DeleteEntityPage.qml in qt_add_qml_module |
| `src/app/resource/translations/opengeolab_zh_CN.ts` | Add translation entries |

---

## Task 1: ShapeStore::replaceShape()

**Files:**
- Modify: `src/libs/geometry/include/opengeolab/geometry/shape_store.hpp:65` (after `remove()`)
- Modify: `src/libs/geometry/src/shape_store.cpp:90` (after `remove()` implementation)
- Modify: `src/libs/geometry/test/shape_store_test.cpp` (append new tests)

- [ ] **Step 1: Write the failing test for replaceShape**

Append to `src/libs/geometry/test/shape_store_test.cpp`:

```cpp
TEST_CASE("ShapeStore replaceShape updates shape and rebuilds sub-shape index") {
    ShapeStore store;
    auto id = store.add("Box6", makeBox(1, 1, 1));

    // Original box: 6 faces, 1 solid
    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    CHECK(entry->faceMap.Extent() == 6);
    CHECK(entry->solidMap.Extent() == 1);

    // Replace with a compound of two boxes
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    builder.Add(compound, BRepPrimAPI_MakeBox(1, 1, 1).Shape());
    builder.Add(compound, BRepPrimAPI_MakeBox(gp_Pnt(5, 0, 0), 2, 2, 2).Shape());
    store.replaceShape(id, compound);

    entry = store.find(id);
    REQUIRE(entry != nullptr);
    CHECK(entry->name == "Box6");
    CHECK(entry->solidMap.Extent() == 2);
    CHECK(entry->faceMap.Extent() == 12);
}

TEST_CASE("ShapeStore replaceShape clears tessellation cache") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());
    store.tessellate(id);
    REQUIRE(store.find(id)->visualData != nullptr);

    store.replaceShape(id, makeBox(2, 2, 2));
    CHECK(store.find(id)->visualData == nullptr);
    CHECK(store.find(id)->triangleTags.empty());
    CHECK(store.find(id)->edgeTags.empty());
    CHECK(store.find(id)->vertexTags.empty());
}

TEST_CASE("ShapeStore replaceShape emits shapeUpdated signal") {
    ShapeStore store;
    auto id = store.add("Box", makeBox());

    uint32_t updated_id = UINT32_MAX;
    auto conn = store.shapeUpdated.connect(
        [&](uint32_t uid, const OpenGeoLab::Geometry::ShapeEntry&) { updated_id = uid; });

    store.replaceShape(id, makeBox(3, 3, 3));
    CHECK(updated_id == id);
}

TEST_CASE("ShapeStore replaceShape throws for unknown shapeId") {
    ShapeStore store;
    CHECK_THROWS_AS(store.replaceShape(999, makeBox()), std::invalid_argument);
}
```

Add these includes at the top of the test file (after the existing includes):

```cpp
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Pnt.hxx>
```

- [ ] **Step 2: Run the test to verify it fails**

Run:
```
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4
```

Expected: Build fails — `replaceShape` is not a member of `ShapeStore`.

- [ ] **Step 3: Add replaceShape declaration to shape_store.hpp**

In `src/libs/geometry/include/opengeolab/geometry/shape_store.hpp`, after the `remove()` declaration (line 65), add:

```cpp
    /**
     * @brief Replace the OCC shape of an existing entry, keeping its id and name.
     * @param shape_id  Target shape
     * @param new_shape Replacement OCC shape
     * @throws std::invalid_argument if shape_id is unknown
     * @post Sub-shape maps are rebuilt; tessellation cache is cleared.
     * @post shapeUpdated signal is emitted.
     */
    void replaceShape(uint32_t shape_id, const TopoDS_Shape& new_shape);
```

- [ ] **Step 4: Implement replaceShape in shape_store.cpp**

In `src/libs/geometry/src/shape_store.cpp`, after `remove()` implementation (after line 90), add:

```cpp
void ShapeStore::replaceShape(uint32_t shape_id, const TopoDS_Shape& new_shape) {
    const ShapeEntry* entry_ptr{};
    {
        const std::lock_guard lock(m_mutex);
        if(shape_id >= m_slots.size() || !m_slots[shape_id]) {
            throw std::invalid_argument("ShapeStore::replaceShape: unknown shapeId");
        }
        auto& entry = *m_slots[shape_id];
        entry.shape = new_shape;

        // Clear old sub-shape maps
        entry.vertexMap.Clear();
        entry.edgeMap.Clear();
        entry.wireMap.Clear();
        entry.faceMap.Clear();
        entry.solidMap.Clear();

        // Clear tessellation cache
        entry.visualData.reset();
        entry.triangleTags.clear();
        entry.edgeTags.clear();
        entry.vertexTags.clear();

        buildSubShapeIndex(entry);
        entry_ptr = &entry;
    }

    shapeUpdated.emit(shape_id, *entry_ptr);
}
```

- [ ] **Step 5: Build and run the tests**

Run:
```
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4
ctest --test-dir build -C RelWithDebInfo -R opengeolab_geometry_test --output-on-failure
```

Expected: All tests pass including the 4 new replaceShape tests.

- [ ] **Step 6: Commit**

```
git add src/libs/geometry/include/opengeolab/geometry/shape_store.hpp \
        src/libs/geometry/src/shape_store.cpp \
        src/libs/geometry/test/shape_store_test.cpp
git commit -m "feat(geometry): add ShapeStore::replaceShape for in-place shape modification

Replaces the OCC shape of an existing entry, rebuilds sub-shape index
maps, clears tessellation cache, and emits shapeUpdated signal. This
enables sub-entity deletion (defeaturing) to modify a shape without
changing its shapeId.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 2: DeleteEntityAction (C++ backend)

**Files:**
- Create: `src/libs/geometry/include/opengeolab/geometry/delete_entity_action.hpp`
- Create: `src/libs/geometry/src/delete_entity_action.cpp`
- Modify: `src/libs/geometry/CMakeLists.txt` (add TKBO + new files)
- Modify: `src/libs/geometry/src/geometry_module.cpp` (register action)
- Create: `src/libs/geometry/test/delete_entity_action_test.cpp`
- Modify: `src/libs/geometry/test/geometry_module_test.cpp:26` (update action count)

### Step 2a: CMake — link TKBO and register new files

- [ ] **Step 1: Update geometry CMakeLists.txt**

In `src/libs/geometry/CMakeLists.txt`:

Add to `geometry_public_headers` (after line 7, the `delete_shape_action.hpp` entry):
```cmake
    include/opengeolab/geometry/delete_entity_action.hpp
```

Add to `geometry_sources` (after line 23, the `src/delete_shape_action.cpp` entry):
```cmake
    src/delete_entity_action.cpp
```

Add `TKBO` to the `PUBLIC_LINKS` section (after line 54, the `TKShHealing)` line). Change line 54 from:
```cmake
    TKShHealing)
```
to:
```cmake
    TKShHealing
    TKBO)
```

Add test file to the `opengeolab_add_doctest_test` (after line 60, the `test/shape_store_test.cpp` entry):
```cmake
        test/delete_entity_action_test.cpp
```

- [ ] **Step 2: Verify CMake configures successfully**

Run:
```
cmake -S . -B build -G Ninja
```

Expected: Configure succeeds (TKBO found). Build will fail since source files don't exist yet.

### Step 2b: Write the header

- [ ] **Step 3: Create delete_entity_action.hpp**

Create `src/libs/geometry/include/opengeolab/geometry/delete_entity_action.hpp`:

```cpp
/**
 * @file delete_entity_action.hpp
 * @brief DeleteEntityAction — removes sub-entities (faces, solids) from shapes
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Removes sub-entities from shapes via defeaturing or compound rebuild.
 *
 * Accepts an array of EntityRef-like objects, groups them by shapeId, and:
 * - GeoFace: uses BRepAlgoAPI_Defeaturing to remove faces and heal the solid.
 * - GeoSolid: rebuilds the compound without the target solids.
 *
 * If all sub-entities are removed, the entire shape is deleted from the store.
 * After modification, the shape is re-tessellated automatically.
 */
class OPENGEOLAB_GEOMETRY_EXPORT DeleteEntityAction final : public Core::IAction {
public:
    explicit DeleteEntityAction(ShapeStore& store);
    ~DeleteEntityAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"delete_entity"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
```

### Step 2c: Write tests first (TDD)

- [ ] **Step 4: Create delete_entity_action_test.cpp**

Create `src/libs/geometry/test/delete_entity_action_test.cpp`:

```cpp
/**
 * @file delete_entity_action_test.cpp
 * @brief Unit tests for DeleteEntityAction — face and solid deletion
 */

#include <opengeolab/geometry/delete_entity_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Pnt.hxx>

#include <doctest/doctest.h>

using OpenGeoLab::Core::NO_PROGRESS_CALLBACK;
using OpenGeoLab::Geometry::DeleteEntityAction;
using OpenGeoLab::Geometry::ShapeStore;

static TopoDS_Shape makeBox(double w = 1.0, double h = 1.0, double d = 1.0) {
    return BRepPrimAPI_MakeBox(w, h, d).Shape();
}

static TopoDS_Shape makeTwoBoxCompound() {
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    builder.Add(compound, BRepPrimAPI_MakeBox(1, 1, 1).Shape());
    builder.Add(compound, BRepPrimAPI_MakeBox(gp_Pnt(5, 0, 0), 2, 2, 2).Shape());
    return compound;
}

TEST_CASE("DeleteEntityAction describe returns valid schema") {
    ShapeStore store;
    DeleteEntityAction action(store);
    auto desc = action.describe();
    CHECK(desc["name"] == "delete_entity");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc["params"].contains("entities"));
}

TEST_CASE("DeleteEntityAction returns error for empty entities array") {
    ShapeStore store;
    DeleteEntityAction action(store);
    const nlohmann::json param = {{"entities", nlohmann::json::array()}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
}

TEST_CASE("DeleteEntityAction returns error for unknown shapeId") {
    ShapeStore store;
    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", 999}, {"type", "GeoFace"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
}

TEST_CASE("DeleteEntityAction returns error for unsupported entity type") {
    ShapeStore store;
    store.add("Box", makeBox());
    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", 0}, {"type", "GeoEdge"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
    auto results = result["results"];
    REQUIRE(results.is_array());
    CHECK(results[0]["status"] == "unsupported");
}

TEST_CASE("DeleteEntityAction removes a face from a box (defeaturing)") {
    ShapeStore store;
    auto id = store.add("Box", makeBox(10, 10, 10));
    store.tessellate(id);
    REQUIRE(store.find(id)->faceMap.Extent() == 6);

    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", id}, {"type", "GeoFace"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);

    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    // After removing one face from a box, the defeatured shape has 5 faces
    CHECK(entry->faceMap.Extent() == 5);
    // Should have been re-tessellated
    CHECK(entry->visualData != nullptr);
}

TEST_CASE("DeleteEntityAction removes a solid from a compound") {
    ShapeStore store;
    auto id = store.add("TwoBoxes", makeTwoBoxCompound());
    store.tessellate(id);
    REQUIRE(store.find(id)->solidMap.Extent() == 2);

    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", id}, {"type", "GeoSolid"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);

    const auto* entry = store.find(id);
    REQUIRE(entry != nullptr);
    CHECK(entry->solidMap.Extent() == 1);
    CHECK(entry->visualData != nullptr);
}

TEST_CASE("DeleteEntityAction removes all solids deletes entire shape") {
    ShapeStore store;
    auto id = store.add("SingleBox", makeBox());
    REQUIRE(store.find(id)->solidMap.Extent() == 1);

    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", id}, {"type", "GeoSolid"}, {"localId", 1}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);

    // Shape should be completely removed
    CHECK(store.find(id) == nullptr);
}

TEST_CASE("DeleteEntityAction returns error for invalid localId") {
    ShapeStore store;
    store.add("Box", makeBox());
    DeleteEntityAction action(store);
    const nlohmann::json param = {
        {"entities", {{{"shapeId", 0}, {"type", "GeoFace"}, {"localId", 999}}}}};
    auto result = action.execute(param, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
}
```

- [ ] **Step 5: Build to verify tests fail (missing implementation)**

Run:
```
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4
```

Expected: Build fails — `DeleteEntityAction` constructor not defined.

### Step 2d: Implement the action

- [ ] **Step 6: Create delete_entity_action.cpp**

Create `src/libs/geometry/src/delete_entity_action.cpp`:

```cpp
/**
 * @file delete_entity_action.cpp
 * @brief DeleteEntityAction — removes sub-entities from shapes
 */

#include <opengeolab/geometry/delete_entity_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/logger.hpp>

#include <BRepAlgoAPI_Defeaturing.hxx>
#include <BRep_Builder.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shape.hxx>

#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {

DeleteEntityAction::DeleteEntityAction(ShapeStore& store) : m_store(store) {}
DeleteEntityAction::~DeleteEntityAction() = default;

nlohmann::json DeleteEntityAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Delete sub-entities (faces, solids) from shapes. "
         "Face removal uses defeaturing (BRepAlgoAPI_Defeaturing). "
         "Solid removal rebuilds the parent compound without the target solids."},
        {"params",
         {{"entities",
           {{"type", "array"},
            {"required", true},
            {"description",
             "Array of {shapeId, type, localId}. "
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

nlohmann::json defeatureFaces(ShapeStore& store, uint32_t shape_id,
                              const std::vector<uint32_t>& face_local_ids) {
    const auto* entry = store.find(shape_id);
    if(entry == nullptr) {
        return {{"shapeId", shape_id}, {"status", "failed"}, {"error", "Shape not found"}};
    }

    TopTools_ListOfShape faces_to_remove;
    for(const auto local_id : face_local_ids) {
        auto face = store.subShape(shape_id, Core::EntityType::GeoFace, local_id);
        if(face.IsNull()) {
            return {{"shapeId", shape_id},
                    {"status", "failed"},
                    {"error", "Invalid face localId: " + std::to_string(local_id)}};
        }
        faces_to_remove.Append(face);
    }

    BRepAlgoAPI_Defeaturing defeaturing;
    defeaturing.SetShape(entry->shape);
    defeaturing.AddFacesToRemove(faces_to_remove);
    defeaturing.SetRunParallel(true);
    defeaturing.SetToFillHistory(true);
    defeaturing.Build();

    if(!defeaturing.IsDone()) {
        return {{"shapeId", shape_id},
                {"status", "failed"},
                {"error", "Defeaturing failed: could not heal the shape after face removal"}};
    }

    const TopoDS_Shape& result = defeaturing.Shape();
    store.replaceShape(shape_id, result);
    store.tessellate(shape_id);

    return {{"shapeId", shape_id},
            {"status", "modified"},
            {"removedFaces", static_cast<int>(face_local_ids.size())}};
}

nlohmann::json removeSolids(ShapeStore& store, uint32_t shape_id,
                            const std::vector<uint32_t>& solid_local_ids) {
    const auto* entry = store.find(shape_id);
    if(entry == nullptr) {
        return {{"shapeId", shape_id}, {"status", "failed"}, {"error", "Shape not found"}};
    }

    const int total_solids = entry->solidMap.Extent();
    const auto num_to_remove = static_cast<int>(solid_local_ids.size());

    // Validate all localIds
    for(const auto local_id : solid_local_ids) {
        if(local_id < 1 || static_cast<int>(local_id) > total_solids) {
            return {{"shapeId", shape_id},
                    {"status", "failed"},
                    {"error", "Invalid solid localId: " + std::to_string(local_id)}};
        }
    }

    // If removing all solids, delete the entire shape
    if(num_to_remove >= total_solids) {
        store.remove(shape_id);
        return {{"shapeId", shape_id},
                {"status", "removed"},
                {"removedSolids", num_to_remove}};
    }

    // Collect solids to remove as a set for fast lookup
    TopTools_IndexedMapOfShape solids_to_remove;
    for(const auto local_id : solid_local_ids) {
        solids_to_remove.Add(entry->solidMap.FindKey(static_cast<int>(local_id)));
    }

    // Rebuild compound without the target solids
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

    return {{"shapeId", shape_id},
            {"status", "modified"},
            {"removedSolids", num_to_remove}};
}

} // namespace

nlohmann::json DeleteEntityAction::execute(const nlohmann::json& param,
                                           const Core::ProgressCallback& progress) {
    if(!param.contains("entities") || !param["entities"].is_array()
       || param["entities"].empty()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "No entities specified"}};
    }

    // Group entities by shapeId
    std::unordered_map<uint32_t, EntityGroup> groups;
    for(const auto& entity_json : param["entities"]) {
        const auto shape_id = entity_json.value("shapeId", static_cast<uint32_t>(0));
        const auto type_str = entity_json.value("type", std::string{});
        const auto local_id = entity_json.value("localId", static_cast<uint32_t>(0));

        const auto entity_type = Core::parseEntityType(type_str);
        if(!entity_type.has_value()) {
            groups[shape_id].hasUnsupported = true;
            continue;
        }

        auto& group = groups[shape_id];
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
        if(group.hasUnsupported && group.faceLocalIds.empty()
           && group.solidLocalIds.empty()) {
            results.push_back(
                {{"shapeId", shape_id},
                 {"status", "unsupported"},
                 {"error", "Edge/Vertex deletion is not supported in v1"}});
            all_ok = false;
            ++processed;
            continue;
        }

        // Validate shape exists
        if(m_store.find(shape_id) == nullptr) {
            results.push_back(
                {{"shapeId", shape_id},
                 {"status", "failed"},
                 {"error", "Unknown shapeId"}});
            all_ok = false;
            ++processed;
            continue;
        }

        // Process faces first (defeaturing), then solids (compound rebuild)
        if(!group.faceLocalIds.empty()) {
            auto face_result = defeatureFaces(m_store, shape_id, group.faceLocalIds);
            if(face_result["status"] == "failed") {
                all_ok = false;
            }
            results.push_back(std::move(face_result));
        }

        // Only process solids if shape still exists (faces may have caused failure)
        if(!group.solidLocalIds.empty() && m_store.find(shape_id) != nullptr) {
            auto solid_result = removeSolids(m_store, shape_id, group.solidLocalIds);
            if(solid_result["status"] == "failed") {
                all_ok = false;
            }
            results.push_back(std::move(solid_result));
        }

        ++processed;
        if(progress) {
            progress(static_cast<double>(processed) / static_cast<double>(total),
                     "Processing...");
        }
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", all_ok}, {"action", ACTION_NAME}, {"results", results}};
}

} // namespace OpenGeoLab::Geometry
```

- [ ] **Step 7: Register action in geometry_module.cpp**

In `src/libs/geometry/src/geometry_module.cpp`, add the include (after line 5, the `delete_shape_action.hpp` include):

```cpp
#include <opengeolab/geometry/delete_entity_action.hpp>
```

Add the registration call (after line 37, the `registerAction<DeleteShapeAction>` line):

```cpp
    registerAction<DeleteEntityAction>(std::ref(m_shapeStore));
```

- [ ] **Step 8: Update geometry_module_test.cpp action count**

In `src/libs/geometry/test/geometry_module_test.cpp`, line 26, change:

```cpp
    CHECK(desc["actions"].size() == 11);
```

to:

```cpp
    CHECK(desc["actions"].size() == 12);
```

- [ ] **Step 9: Build and run all geometry tests**

Run:
```
cmake --build build --config RelWithDebInfo --target opengeolab_geometry_test --parallel 4
ctest --test-dir build -C RelWithDebInfo -R opengeolab_geometry_test --output-on-failure
```

Expected: All tests pass. The defeaturing test ("removes a face from a box") confirms OCC `BRepAlgoAPI_Defeaturing` works correctly with the linked `TKBO` library.

- [ ] **Step 10: Commit**

```
git add src/libs/geometry/include/opengeolab/geometry/delete_entity_action.hpp \
        src/libs/geometry/src/delete_entity_action.cpp \
        src/libs/geometry/test/delete_entity_action_test.cpp \
        src/libs/geometry/CMakeLists.txt \
        src/libs/geometry/src/geometry_module.cpp \
        src/libs/geometry/test/geometry_module_test.cpp
git commit -m "feat(geometry): add DeleteEntityAction for face and solid deletion

New delete_entity action removes sub-entities from shapes:
- GeoFace: BRepAlgoAPI_Defeaturing removes faces and heals the solid
- GeoSolid: compound rebuild or full shape removal
- Unsupported types (edge/vertex) return explicit error

Links TKBO for defeaturing support. Auto re-tessellates after modification.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 3: DeleteEntityPage QML

**Files:**
- Create: `src/app/resource/qml/components/pages/DeleteEntityPage.qml`
- Modify: `src/app/resource/qml/MainPages.qml:22` (add componentMap entry)
- Modify: `src/app/resource/qml/RibbonConfig.qml:43-58` (add Delete to Modify group)
- Modify: `src/app/CMakeLists.txt:103` (register new QML file)

- [ ] **Step 1: Create DeleteEntityPage.qml**

Create `src/app/resource/qml/components/pages/DeleteEntityPage.qml`:

```qml
import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../.."
import ".."

/**
 * Delete Entity page — activates pick mode for face/solid and
 * submits delete_entity requests for the selected entities.
 */
FunctionPageBase {
    id: root

    pageTitle: qsTr("Delete Entity")
    pageIcon: "trash"
    actionId: "deleteEntity"

    function open(payload) {
        root.x = 292;
        root.y = 0;
        root.pageVisible = true;
        root.forceActiveFocus();
        sceneCommand("clear_selection", {});
        sceneCommand("set_pick_mode", {
            pickMask: typeSelector.mask,
            enabled: true
        });
    }

    function close() {
        sceneCommand("set_pick_mode", { enabled: false });
        sceneCommand("clear_selection", {});
        root.pageVisible = false;
        if (MainPages.currentOpenPage === root.actionId) {
            MainPages.currentOpenPage = "";
        }
    }

    function getParameters() {
        var entities = [];
        for (var i = 0; i < SelectionService.selections.length; ++i) {
            var sel = SelectionService.selections[i];
            entities.push({
                shapeId: sel.shapeId,
                type: entityTypeTag(sel.entityType),
                localId: sel.localId
            });
        }
        return {
            module: "geometry",
            action: "delete_entity",
            param: { entities: entities },
            mute: false
        };
    }

    function sceneCommand(action, param) {
        RequestService.submitAsync(JSON.stringify({
            module: "scene",
            action: action,
            param: param ?? {},
            mute: true
        }));
    }

    function entityTypeTag(typeInt) {
        var map = {
            3: "GeoFace",
            4: "GeoSolid"
        };
        return map[typeInt] ?? "GeoFace";
    }

    // ── Entity type selector (Face + Solid only) ───────────────────
    Text {
        text: qsTr("Entity Filter")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    EntityTypeSelector {
        id: typeSelector

        width: parent.width
        theme: root.theme
        mask: 8
        typeModel: [
            { label: qsTr("Face"),  icon: "entityFace",  mask: 8 },
            { label: qsTr("Solid"), icon: "entitySolid", mask: 16 }
        ]
        exclusiveMasks: [16]

        onMaskChanged: {
            sceneCommand("set_pick_mode", { pickMask: typeSelector.mask });
        }
    }

    // ── Pick mode indicator ────────────────────────────────────────
    Rectangle {
        width: parent.width
        height: 24
        radius: root.theme.radiusSmall
        color: SelectionService.pickEnabled
            ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.14 : 0.08)
            : "transparent"
        visible: SelectionService.pickEnabled

        Row {
            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                id: pulsingDot

                width: 6
                height: 6
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: root.theme.accentA

                SequentialAnimation on opacity {
                    running: SelectionService.pickEnabled
                    loops: Animation.Infinite

                    NumberAnimation {
                        from: 1.0; to: 0.3; duration: 800
                        easing.type: Easing.InOutQuad
                    }
                    NumberAnimation {
                        from: 0.3; to: 1.0; duration: 800
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            Text {
                text: qsTr("Click to select · Right-click to deselect")
                color: root.theme.accentA
                font.pixelSize: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // ── Selection count ────────────────────────────────────────────
    Text {
        text: qsTr("Selected: %1").arg(SelectionService.selections.length)
        color: root.theme.textSecondary
        font.pixelSize: 12
        visible: SelectionService.selections.length > 0
    }

    // ── Selected entity chips ──────────────────────────────────────
    Flickable {
        id: chipFlickable

        width: parent.width
        height: Math.min(chipFlow.implicitHeight, 120)
        contentHeight: chipFlow.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        visible: SelectionService.selections.length > 0

        Flow {
            id: chipFlow

            width: chipFlickable.width
            spacing: 6

            Repeater {
                model: SelectionService.selections

                EntityChip {
                    required property var modelData

                    theme: root.theme
                    shapeId: modelData.shapeId
                    entityType: modelData.entityType
                    localId: modelData.localId

                    onRemoveRequested: function(sid, etype, lid) {
                        sceneCommand("deselect", {
                            entities: [{ shapeId: sid, type: root.entityTypeTag(etype), localId: lid }]
                        });
                    }
                }
            }
        }
    }

    // ── Empty state ────────────────────────────────────────────────
    Rectangle {
        width: parent.width
        height: 48
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted
        visible: SelectionService.selections.length === 0

        Text {
            anchors.centerIn: parent
            text: SelectionService.pickEnabled
                ? qsTr("Click faces or solids in the viewport to select.\nRight-click to remove from selection.")
                : qsTr("No entities selected.\nActivate pick mode to begin.")
            color: root.theme.textTertiary
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
            lineHeight: 1.3
        }
    }

    // ── Clear All ──────────────────────────────────────────────────
    Rectangle {
        width: parent.width
        height: 32
        radius: root.theme.radiusSmall
        visible: SelectionService.selections.length > 0
        color: clearMouse.pressed
            ? root.theme.surfaceStrong
            : (clearMouse.containsMouse
                ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.96 : 0.9)
                : root.theme.surfaceMuted)
        border.width: 1
        border.color: clearMouse.containsMouse
            ? root.theme.tint(root.theme.danger, root.theme.darkMode ? 0.56 : 0.36)
            : root.theme.borderSubtle

        Behavior on color {
            ColorAnimation { duration: root.theme.animFast }
        }

        Text {
            anchors.centerIn: parent
            text: qsTr("Clear All")
            color: clearMouse.containsMouse ? root.theme.danger : root.theme.textPrimary
            font.pixelSize: 12
            font.bold: true
        }

        MouseArea {
            id: clearMouse

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: sceneCommand("clear_selection", {})
        }
    }
}
```

- [ ] **Step 2: Register in MainPages.qml**

In `src/app/resource/qml/MainPages.qml`, add to the `componentMap` object (after line 22, the `"queryMesh"` entry):

```js
        "deleteEntity": { path: "components/pages/DeleteEntityPage.qml" },
```

Note: insert before the closing `})` on line 23.

- [ ] **Step 3: Add Delete button to RibbonConfig.qml**

In `src/app/resource/qml/RibbonConfig.qml`, add a new action to the Modify group's `"actions"` array. After line 57 (the closing `}` of the Offset action), add a comma and the new entry:

```js
                    ,{
                        "key": "deleteEntity",
                        "title": qsTr("Delete"),
                        "icon": "trash",
                        "accentOne": "accentD",
                        "accentTwo": "accentC"
                    }
```

- [ ] **Step 4: Register QML file in app CMakeLists.txt**

In `src/app/CMakeLists.txt`, add the new QML file to the `QML_FILES` list (after line 103, the `MeshQueryPage.qml` entry):

```cmake
    resource/qml/components/pages/DeleteEntityPage.qml
```

- [ ] **Step 5: Build the app to verify QML registration**

Run:
```
cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4
```

Expected: Build succeeds. The DeleteEntityPage is accessible from the Geometry → Modify → Delete ribbon button.

- [ ] **Step 6: Commit**

```
git add src/app/resource/qml/components/pages/DeleteEntityPage.qml \
        src/app/resource/qml/MainPages.qml \
        src/app/resource/qml/RibbonConfig.qml \
        src/app/CMakeLists.txt
git commit -m "feat(app): add DeleteEntityPage with face/solid picker workflow

New function page in Geometry → Modify → Delete ribbon button:
- EntityTypeSelector configured for Face + Solid only
- Pick mode activation with mask-based filtering
- Entity chips display with remove capability
- Submits delete_entity requests to geometry module

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 4: Sidebar right-click context menu

**Files:**
- Modify: `src/app/resource/qml/components/ShapeListItem.qml:56-59` (add right-click handler)

- [ ] **Step 1: Add context menu to ShapeListItem.qml**

In `src/app/resource/qml/components/ShapeListItem.qml`, add a `signal deleteShapeRequested` and the context menu implementation.

First, add the signal after line 23 (`signal toggleMeshVisibility`):

```qml
    signal deleteShapeRequested(int shapeId)
```

Then, add a `Menu` import at the top of the file. After line 3 (`import QtQuick.Layouts`), add:

```qml
import QtQuick.Controls.Basic
```

Next, replace the existing `MouseArea` inside the clickable header (lines 56-59):

```qml
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.expanded = !root.expanded
            }
```

with:

```qml
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                cursorShape: Qt.PointingHandCursor
                onClicked: function(mouse) {
                    if (mouse.button === Qt.RightButton) {
                        contextMenu.popup();
                    } else {
                        root.expanded = !root.expanded;
                    }
                }
            }

            Menu {
                id: contextMenu

                MenuItem {
                    text: qsTr("Delete Shape")
                    icon.source: Qt.resolvedUrl("../../icons/trash.svg")
                    onTriggered: root.deleteShapeRequested(root.shapeId)
                }
            }
```

- [ ] **Step 2: Connect the signal in SidebarPanel.qml**

In `src/app/resource/qml/sections/SidebarPanel.qml`, add the `onDeleteShapeRequested` handler to the `ShapeListItem` delegate. After line 226 (`onToggleMeshVisibility: (sid) => root.toggleMeshVisibility(sid)`), add:

```qml
                onDeleteShapeRequested: function(sid) {
                    RequestService.submitAsync(JSON.stringify({
                        module: "geometry",
                        action: "delete_shape",
                        param: { shapeId: sid },
                        mute: false
                    }));
                }
```

- [ ] **Step 3: Build and verify**

Run:
```
cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4
```

Expected: Build succeeds. Right-clicking a shape in the sidebar shows a context menu with "Delete Shape".

- [ ] **Step 4: Commit**

```
git add src/app/resource/qml/components/ShapeListItem.qml \
        src/app/resource/qml/sections/SidebarPanel.qml
git commit -m "feat(app): add right-click delete to sidebar ShapeListItem

Right-clicking a shape in the Scene Explorer now shows a context menu
with 'Delete Shape' option. Uses existing delete_shape action via
RequestService.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 5: Translations (i18n)

**Files:**
- Modify: `src/app/resource/translations/opengeolab_zh_CN.ts`

- [ ] **Step 1: Add translation entries**

In `src/app/resource/translations/opengeolab_zh_CN.ts`, add new contexts.

In the `RibbonConfig` context, add:

```xml
        <message>
            <source>Delete</source>
            <translation>删除</translation>
        </message>
```

Add a new `DeleteEntityPage` context (before the closing `</TS>` tag):

```xml
    <context>
        <name>DeleteEntityPage</name>
        <message>
            <source>Delete Entity</source>
            <translation>删除实体</translation>
        </message>
        <message>
            <source>Entity Filter</source>
            <translation>实体过滤</translation>
        </message>
        <message>
            <source>Click to select · Right-click to deselect</source>
            <translation>左键选中 · 右键取消</translation>
        </message>
        <message>
            <source>Selected: %1</source>
            <translation>已选中: %1</translation>
        </message>
        <message>
            <source>Face</source>
            <translation>面</translation>
        </message>
        <message>
            <source>Solid</source>
            <translation>实体</translation>
        </message>
        <message>
            <source>Click faces or solids in the viewport to select.
Right-click to remove from selection.</source>
            <translation>在视口中点击面或实体以选中。
右键移除选中。</translation>
        </message>
        <message>
            <source>No entities selected.
Activate pick mode to begin.</source>
            <translation>未选中任何实体。
激活拾取模式以开始。</translation>
        </message>
        <message>
            <source>Clear All</source>
            <translation>全部清除</translation>
        </message>
    </context>
    <context>
        <name>ShapeListItem</name>
        <message>
            <source>Delete Shape</source>
            <translation>删除形体</translation>
        </message>
    </context>
```

- [ ] **Step 2: Build to verify translations compile**

Run:
```
cmake --build build --config RelWithDebInfo --target opengeolab_app --parallel 4
```

Expected: Build succeeds with updated translations.

- [ ] **Step 3: Commit**

```
git add src/app/resource/translations/opengeolab_zh_CN.ts
git commit -m "docs(i18n): add Chinese translations for delete entity feature

Adds zh_CN translations for DeleteEntityPage, RibbonConfig Delete
button, and ShapeListItem context menu.

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
```

---

## Task 6: Full integration build and test

- [ ] **Step 1: Run full build**

```
cmake --build build --config RelWithDebInfo --parallel 4
```

Expected: Full project builds without errors or warnings.

- [ ] **Step 2: Run full test suite**

```
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Expected: All tests pass, including:
- `opengeolab_geometry_test` (replaceShape tests, delete_entity tests, updated action count)
- All other existing test suites remain green

- [ ] **Step 3: Final commit (if any fixups needed)**

Only if fixups were needed. Otherwise skip.
