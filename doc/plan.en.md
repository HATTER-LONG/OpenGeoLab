# OpenGeoLab Architecture Snapshot and Next-Step Plan

## 1. Current Project State

The repository has already been reorganized into a modular scaffold:

- apps/OpenGeoLabApp
- libs/core
- libs/geometry
- libs/mesh
- libs/scene
- libs/render
- libs/selection
- libs/command
- python/python_wrapper

The first fully wired vertical slice is now:

QML UI -> app controller -> Python bridge -> Kangaroo ComponentFactory -> geometry lib

Its main value is not geometry capability yet. Its value is that the cross-layer request path, module boundary rules, and scriptable entrypoint are now stable for every future module.

## 2. Current Module Responsibilities

### 2.1 apps/OpenGeoLabApp

- Hosts the QML UI and app-level controller
- Accepts module + JSON requests from QML
- Does not depend on geometry-specific service interfaces

### 2.2 libs/core

- Defines IService and ServiceResponse
- Provides ComponentRequestDispatcher
- Resolves services through Kangaroo ComponentFactory by module name

### 2.3 libs/geometry

- Currently provides PlaceholderGeometryModel
- Registers the geometry service through registerGeometryComponents
- Will later be replaced by OCC-backed GeometryModel, import, topology, and cleanup capabilities

### 2.4 python/python_wrapper

- Provides OpenGeoLabPythonBridge
- Is shared by the app layer and the pybind11 module
- Centralizes the high-level call(module, params) contract

### 2.5 libs/scene

- Organizes placeholder geometry into a stable scene-graph semantic layer
- Provides node IDs, display labels, and selectable objects for render and selection

### 2.6 libs/render

- Converts scene-graph state into a placeholder RenderFrame
- Owns viewport size, camera pose, draw items, and highlight-oriented render data

### 2.7 libs/selection

- Produces placeholder pick and box-select results from RenderFrame and scene data
- Already demonstrates the geometry -> scene -> render -> selection core interaction flow

## 3. Architecture Assessment for QML + OpenGL + Mouse Interaction + Python Script Recording

Conclusion: the current direction is sound, and placeholder scene/render/selection implementations now exist. The remaining bottlenecks are a real OpenGL viewport host, command ownership, and semantic script recording.

### 3.1 Why the current direction is correct

- The UI is not directly coupled to the geometry kernel.
- The Python bridge already acts as a high-level automation entrypoint, which is the right place for script replay and LLM-generated workflows.
- Kangaroo ComponentFactory provides a consistent module-string routing model across geometry, mesh, scene, selection, and command.
- The render layer is explicitly kept separate from geometry and mesh kernels, which is the correct prerequisite for OpenGL view ownership and picking.

### 3.2 What is still missing

- The render module does not yet own a real OpenGL viewport host or GPU resource cache layer.
- The scene module now has a placeholder scene graph, but it is not yet the single source of truth for full visibility and interaction state.
- The selection module now exposes placeholder pick and box-select semantics, but it does not yet implement ray picking, ID-buffer picking, or rectangle selection algorithms.
- The command module does not yet own user-visible operations, so undo/redo and Python recording are not available yet.

### 3.3 Recommended interaction layering

Recommended split of responsibilities:

1. QML layer
   - Layout, tool state, and event forwarding only
   - No geometry, picking, or rendering algorithms

2. app / InteractionController
   - Normalizes mouse press, move, release, and wheel events
   - Converts them into semantic requests such as orbit camera, pick face, or box select

3. scene module
   - Owns visible nodes, visibility state, active object state, and selection set
   - Acts as the semantic layer shared by rendering and selection

4. render module
   - Consumes RenderData exported from scene
   - Owns GPU buffers, camera state, render passes, and highlight rendering
   - Must not directly consume OCC or Gmsh entities

5. selection module
   - Implements ray casting, GPU picking, and rectangle selection
   - Returns entity IDs or scene node IDs
   - Does not directly mutate the UI

6. command module
   - Owns user-visible operations as commands
   - Records high-level actions for undo/redo and Python script generation

### 3.4 Key principle for Python script recording

Script recording should never store raw mouse motion as the main artifact.

Correct granularity:

- Camera orbit -> set_camera_pose
- Face picking -> select_face or select_entities
- Box selection -> box_select(screen_rect, filter)
- Geometry cleanup -> geometry.removeSmallFaces(params)
- Mesh generation -> mesh.generateSurface(params)

In other words, mouse input should trigger commands, and commands should generate Python operations. The script layer must capture intent, not incidental UI coordinates.

## 4. Current CMake Assessment

The current CMake layout is significantly better than the original monolithic setup:

- The root CMake file owns dependencies and global options
- Each module owns its own CMakeLists.txt
- The geometry smoke test now lives in libs/geometry/tests
- First-party libraries can switch between static and shared builds through OPENGEOLAB_BUILD_SHARED_LIBS
- core and geometry now use generated export headers for Windows/MSVC shared-library compatibility
- Basic install/export rules are now present for future reusable-library packaging

Rules that should remain in place:

- Every public library module must own an export header
- Unit tests must stay inside their owning module
- Only future cross-module integration scenarios should use a top-level tests directory

## 5. Recommended Next Implementation Order

### Phase A: introduce a real OpenGL viewport host

Goal: make QML capable of driving orbit, pan, zoom, and redraw.

### Phase B: let the command system own user-visible operations

Goal: create a single path for undo/redo and Python script recording.

### Phase C: replace the geometry placeholder with a real OCC-backed model path

Goal: move from a demonstration slice to an actual editable geometry module.
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\scene\include\ogl\scene\PlaceholderSceneGraph.hpp
/**
 * @file PlaceholderSceneGraph.hpp
 * @brief Placeholder scene graph types used to validate scene-layer data flow.
 */

#pragma once

#include <ogl/geometry/PlaceholderGeometryModel.hpp>
#include <ogl/scene/export.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace ogl::scene {

/**
 * @brief Placeholder scene node generated from the geometry placeholder model.
 */
struct OGL_SCENE_EXPORT PlaceholderSceneNode {
   std::string nodeId;
   std::string displayName;
   std::string renderPrimitive;
   int conceptualBodyIndex{0};
   bool selectable{true};

   [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Lightweight scene graph that stabilizes IDs between geometry and render layers.
 */
class OGL_SCENE_EXPORT PlaceholderSceneGraph {
public:
   PlaceholderSceneGraph(std::string scene_id, std::string model_name,
                    std::vector<PlaceholderSceneNode> nodes);

   [[nodiscard]] auto sceneId() const -> const std::string&;
   [[nodiscard]] auto modelName() const -> const std::string&;
   [[nodiscard]] auto nodes() const -> const std::vector<PlaceholderSceneNode>&;
   [[nodiscard]] auto summary() const -> std::string;
   [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
   std::string m_sceneId;
   std::string m_modelName;
   std::vector<PlaceholderSceneNode> m_nodes;
};

/**
 * @brief Build a placeholder scene graph from the geometry-layer placeholder model.
 * @param geometry_model Placeholder geometry model.
 * @return Stable scene graph representation.
 */
OGL_SCENE_EXPORT auto buildPlaceholderSceneGraph(
   const ogl::geometry::PlaceholderGeometryModel& geometry_model) -> PlaceholderSceneGraph;

} // namespace ogl::scene
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\scene\include\ogl\scene\SceneComponentRegistration.hpp
/**
 * @file SceneComponentRegistration.hpp
 * @brief Registers placeholder scene services into Kangaroo ComponentFactory.
 */

#pragma once

#include <ogl/scene/export.hpp>

namespace ogl::scene {

/**
 * @brief Register scene services exactly once for the current process.
 */
OGL_SCENE_EXPORT void registerSceneComponents();

} // namespace ogl::scene
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\scene\src\PlaceholderSceneGraph.cpp
#include <ogl/scene/PlaceholderSceneGraph.hpp>

#include <sstream>
#include <utility>

namespace {

auto buildNodeId(const std::string& model_name, int body_index) -> std::string {
   std::ostringstream stream;
   stream << model_name << "::body_" << body_index;
   return stream.str();
}

} // namespace

namespace ogl::scene {

auto PlaceholderSceneNode::toJson() const -> nlohmann::json {
   return {{"nodeId", nodeId},
         {"displayName", displayName},
         {"renderPrimitive", renderPrimitive},
         {"conceptualBodyIndex", conceptualBodyIndex},
         {"selectable", selectable}};
}

PlaceholderSceneGraph::PlaceholderSceneGraph(std::string scene_id, std::string model_name,
                                  std::vector<PlaceholderSceneNode> nodes)
   : m_sceneId(std::move(scene_id)),
     m_modelName(std::move(model_name)),
     m_nodes(std::move(nodes)) {}

auto PlaceholderSceneGraph::sceneId() const -> const std::string& { return m_sceneId; }

auto PlaceholderSceneGraph::modelName() const -> const std::string& { return m_modelName; }

auto PlaceholderSceneGraph::nodes() const -> const std::vector<PlaceholderSceneNode>& {
   return m_nodes;
}

auto PlaceholderSceneGraph::summary() const -> std::string {
   std::ostringstream stream;
   stream << "Placeholder scene graph '" << m_sceneId << "' stabilizes " << m_nodes.size()
         << " selectable nodes for geometry model '" << m_modelName << "'.";
   return stream.str();
}

auto PlaceholderSceneGraph::toJson() const -> nlohmann::json {
   nlohmann::json nodes_json = nlohmann::json::array();
   for(const auto& node : m_nodes) {
      nodes_json.push_back(node.toJson());
   }

   return {{"sceneId", m_sceneId},
         {"modelName", m_modelName},
         {"nodeCount", m_nodes.size()},
         {"nodes", std::move(nodes_json)},
         {"summary", summary()}};
}

auto buildPlaceholderSceneGraph(const ogl::geometry::PlaceholderGeometryModel& geometry_model)
   -> PlaceholderSceneGraph {
   std::vector<PlaceholderSceneNode> nodes;
   nodes.reserve(static_cast<std::size_t>(geometry_model.bodyCount()));

   for(int body_index = 1; body_index <= geometry_model.bodyCount(); ++body_index) {
      nodes.push_back({.nodeId = buildNodeId(geometry_model.modelName(), body_index),
                   .displayName = geometry_model.modelName() + " Body " +
                              std::to_string(body_index),
                   .renderPrimitive = body_index % 2 == 0 ? "wire-overlay" : "solid-body",
                   .conceptualBodyIndex = body_index,
                   .selectable = true});
   }

   return PlaceholderSceneGraph(geometry_model.modelName() + "::scene", geometry_model.modelName(),
                         std::move(nodes));
}

} // namespace ogl::scene
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\scene\src\SceneComponentRegistration.cpp
#include <ogl/scene/SceneComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/geometry/PlaceholderGeometryModel.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/logger_factory.hpp>

#include <mutex>
#include <sstream>

namespace {

auto sceneLogger() {
   static auto logger = Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab.Scene");
   return logger;
}

auto buildSceneEquivalentPython(const nlohmann::json& params) -> std::string {
   std::ostringstream script;
   script << "import opengeolab\n\n";
   script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
   script << "result = bridge.call(\"scene\", R\"JSON(" << params.dump(2) << ")JSON\")\n";
   script << "print(result)";
   return script.str();
}

auto buildGeometryModel(const nlohmann::json& params) -> ogl::geometry::PlaceholderGeometryModel {
   return ogl::geometry::PlaceholderGeometryModel(
      {.modelName = params.value("modelName", std::string{"Bracket_A01"}),
       .bodyCount = params.value("bodyCount", 3),
       .source = params.value("source", std::string{"scene-service"})});
}

class PlaceholderSceneService final : public ogl::core::IService {
public:
   auto processRequest(const std::string& module_name, const nlohmann::json& params)
      -> ogl::core::ServiceResponse override {
      const std::string operation_name = params.value("operation", std::string{"unknown"});
      if(module_name != "scene") {
         return {.success = false,
               .moduleName = module_name,
               .operationName = operation_name,
               .message = "Placeholder scene service only accepts the scene module.",
               .payload = nlohmann::json::object()};
      }

      if(operation_name != "buildPlaceholderScene") {
         return {.success = false,
               .moduleName = module_name,
               .operationName = operation_name,
               .message = "Unsupported scene operation. Use buildPlaceholderScene.",
               .payload = nlohmann::json::object()};
      }

      const auto geometry_model = buildGeometryModel(params);
      const auto scene_graph = ogl::scene::buildPlaceholderSceneGraph(geometry_model);
      auto logger = sceneLogger();
      logger->info("Built placeholder scene graph sceneId={} nodeCount={}", scene_graph.sceneId(),
                scene_graph.nodes().size());

      return {.success = true,
            .moduleName = module_name,
            .operationName = operation_name,
            .message = "Placeholder scene graph assembled from geometry model.",
            .payload = {{"componentId", "scene"},
                     {"geometryModel", geometry_model.toJson()},
                     {"sceneGraph", scene_graph.toJson()},
                     {"summary", scene_graph.summary()},
                     {"equivalentPython", buildSceneEquivalentPython(params)}}};
   }
};

class SceneServiceFactory final : public ogl::core::IServiceSingletonFactory {
public:
   auto instance() const -> tObjectSharedPtr override {
      static auto service = std::make_shared<PlaceholderSceneService>();
      return service;
   }
};

} // namespace

namespace ogl::scene {

void registerSceneComponents() {
   static std::once_flag once;
   std::call_once(once, []() {
      auto logger = sceneLogger();
      g_ComponentFactory.registInstanceFactoryWithID<SceneServiceFactory>("scene");
      logger->info("Registered scene component factory for placeholder scene graph pipeline");
   });
}

} // namespace ogl::scene
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\render\include\ogl\render\PlaceholderRenderFrame.hpp
/**
 * @file PlaceholderRenderFrame.hpp
 * @brief Placeholder render-frame types used to validate render-layer data flow.
 */

#pragma once

#include <ogl/render/export.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace ogl::render {

/**
 * @brief Minimal camera state carried by the placeholder render frame.
 */
struct OGL_RENDER_EXPORT PlaceholderCameraPose {
   double yawDegrees{32.0};
   double pitchDegrees{-18.0};
   double distance{8.5};

   [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Single renderable item mapped from a placeholder scene node.
 */
struct OGL_RENDER_EXPORT PlaceholderDrawItem {
   std::string nodeId;
   std::string pipelineKey;
   std::string colorHex;
   bool highlighted{false};

   [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Lightweight render-frame description produced from scene-layer data.
 */
class OGL_RENDER_EXPORT PlaceholderRenderFrame {
public:
   PlaceholderRenderFrame(std::string frame_id, std::string scene_id, int viewport_width,
                     int viewport_height, PlaceholderCameraPose camera,
                     std::vector<PlaceholderDrawItem> draw_items);

   [[nodiscard]] auto frameId() const -> const std::string&;
   [[nodiscard]] auto sceneId() const -> const std::string&;
   [[nodiscard]] auto viewportWidth() const -> int;
   [[nodiscard]] auto viewportHeight() const -> int;
   [[nodiscard]] auto camera() const -> const PlaceholderCameraPose&;
   [[nodiscard]] auto drawItems() const -> const std::vector<PlaceholderDrawItem>&;
   [[nodiscard]] auto summary() const -> std::string;
   [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
   std::string m_frameId;
   std::string m_sceneId;
   int m_viewportWidth{0};
   int m_viewportHeight{0};
   PlaceholderCameraPose m_camera;
   std::vector<PlaceholderDrawItem> m_drawItems;
};

/**
 * @brief Convert a placeholder scene graph into placeholder render-frame state.
 * @param scene_graph Scene-layer placeholder graph.
 * @param params Optional request parameters such as viewport size and highlight target.
 * @return Placeholder render-frame description.
 */
OGL_RENDER_EXPORT auto buildPlaceholderRenderFrame(const ogl::scene::PlaceholderSceneGraph& scene_graph,
                                       const nlohmann::json& params)
   -> PlaceholderRenderFrame;

} // namespace ogl::render
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\render\include\ogl\render\RenderComponentRegistration.hpp
/**
 * @file RenderComponentRegistration.hpp
 * @brief Registers placeholder render services into Kangaroo ComponentFactory.
 */

#pragma once

#include <ogl/render/export.hpp>

namespace ogl::render {

/**
 * @brief Register render services exactly once for the current process.
 */
OGL_RENDER_EXPORT void registerRenderComponents();

} // namespace ogl::render
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\render\src\PlaceholderRenderFrame.cpp
#include <ogl/render/PlaceholderRenderFrame.hpp>

#include <algorithm>
#include <array>
#include <sstream>
#include <utility>

namespace {

constexpr std::array<const char*, 4> kPlaceholderPalette = {"#4f7b6b", "#b9854c", "#7089a3",
                                              "#7f5d86"};

} // namespace

namespace ogl::render {

auto PlaceholderCameraPose::toJson() const -> nlohmann::json {
   return {{"yawDegrees", yawDegrees}, {"pitchDegrees", pitchDegrees}, {"distance", distance}};
}

auto PlaceholderDrawItem::toJson() const -> nlohmann::json {
   return {{"nodeId", nodeId},
         {"pipelineKey", pipelineKey},
         {"colorHex", colorHex},
         {"highlighted", highlighted}};
}

PlaceholderRenderFrame::PlaceholderRenderFrame(std::string frame_id, std::string scene_id,
                                    int viewport_width, int viewport_height,
                                    PlaceholderCameraPose camera,
                                    std::vector<PlaceholderDrawItem> draw_items)
   : m_frameId(std::move(frame_id)),
     m_sceneId(std::move(scene_id)),
     m_viewportWidth(viewport_width),
     m_viewportHeight(viewport_height),
     m_camera(std::move(camera)),
     m_drawItems(std::move(draw_items)) {}

auto PlaceholderRenderFrame::frameId() const -> const std::string& { return m_frameId; }

auto PlaceholderRenderFrame::sceneId() const -> const std::string& { return m_sceneId; }

auto PlaceholderRenderFrame::viewportWidth() const -> int { return m_viewportWidth; }

auto PlaceholderRenderFrame::viewportHeight() const -> int { return m_viewportHeight; }

auto PlaceholderRenderFrame::camera() const -> const PlaceholderCameraPose& { return m_camera; }

auto PlaceholderRenderFrame::drawItems() const -> const std::vector<PlaceholderDrawItem>& {
   return m_drawItems;
}

auto PlaceholderRenderFrame::summary() const -> std::string {
   std::ostringstream stream;
   stream << "Placeholder render frame '" << m_frameId << "' prepared " << m_drawItems.size()
         << " draw items for viewport " << m_viewportWidth << "x" << m_viewportHeight << ".";
   return stream.str();
}

auto PlaceholderRenderFrame::toJson() const -> nlohmann::json {
   nlohmann::json draw_items_json = nlohmann::json::array();
   for(const auto& draw_item : m_drawItems) {
      draw_items_json.push_back(draw_item.toJson());
   }

   return {{"frameId", m_frameId},
         {"sceneId", m_sceneId},
         {"viewportWidth", m_viewportWidth},
         {"viewportHeight", m_viewportHeight},
         {"camera", m_camera.toJson()},
         {"drawItemCount", m_drawItems.size()},
         {"drawItems", std::move(draw_items_json)},
         {"summary", summary()}};
}

auto buildPlaceholderRenderFrame(const ogl::scene::PlaceholderSceneGraph& scene_graph,
                         const nlohmann::json& params) -> PlaceholderRenderFrame {
   const int viewport_width = std::max(params.value("viewportWidth", 1280), 64);
   const int viewport_height = std::max(params.value("viewportHeight", 720), 64);
   const std::string highlighted_node_id = params.value("highlightNodeId", std::string{});

   PlaceholderCameraPose camera{.yawDegrees = params.value("cameraYawDegrees", 32.0),
                         .pitchDegrees = params.value("cameraPitchDegrees", -18.0),
                         .distance = params.value("cameraDistance", 8.5)};

   std::vector<PlaceholderDrawItem> draw_items;
   draw_items.reserve(scene_graph.nodes().size());

   for(std::size_t index = 0; index < scene_graph.nodes().size(); ++index) {
      const auto& node = scene_graph.nodes()[index];
      const bool highlighted = !highlighted_node_id.empty() && highlighted_node_id == node.nodeId;
      draw_items.push_back({.nodeId = node.nodeId,
                       .pipelineKey = node.renderPrimitive,
                       .colorHex = kPlaceholderPalette[index % kPlaceholderPalette.size()],
                       .highlighted = highlighted});
   }

   return PlaceholderRenderFrame(scene_graph.sceneId() + "::frame", scene_graph.sceneId(),
                          viewport_width, viewport_height, camera, std::move(draw_items));
}

} // namespace ogl::render
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\render\src\RenderComponentRegistration.cpp
#include <ogl/render/RenderComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/geometry/PlaceholderGeometryModel.hpp>
#include <ogl/render/PlaceholderRenderFrame.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/logger_factory.hpp>

#include <mutex>
#include <sstream>

namespace {

auto renderLogger() {
   static auto logger = Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab.Render");
   return logger;
}

auto buildRenderEquivalentPython(const nlohmann::json& params) -> std::string {
   std::ostringstream script;
   script << "import opengeolab\n\n";
   script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
   script << "result = bridge.call(\"render\", R\"JSON(" << params.dump(2) << ")JSON\")\n";
   script << "print(result)";
   return script.str();
}

auto buildGeometryModel(const nlohmann::json& params) -> ogl::geometry::PlaceholderGeometryModel {
   return ogl::geometry::PlaceholderGeometryModel(
      {.modelName = params.value("modelName", std::string{"Bracket_A01"}),
       .bodyCount = params.value("bodyCount", 3),
       .source = params.value("source", std::string{"render-service"})});
}

class PlaceholderRenderService final : public ogl::core::IService {
public:
   auto processRequest(const std::string& module_name, const nlohmann::json& params)
      -> ogl::core::ServiceResponse override {
      const std::string operation_name = params.value("operation", std::string{"unknown"});
      if(module_name != "render") {
         return {.success = false,
               .moduleName = module_name,
               .operationName = operation_name,
               .message = "Placeholder render service only accepts the render module.",
               .payload = nlohmann::json::object()};
      }

      if(operation_name != "buildPlaceholderFrame") {
         return {.success = false,
               .moduleName = module_name,
               .operationName = operation_name,
               .message = "Unsupported render operation. Use buildPlaceholderFrame.",
               .payload = nlohmann::json::object()};
      }

      const auto geometry_model = buildGeometryModel(params);
      const auto scene_graph = ogl::scene::buildPlaceholderSceneGraph(geometry_model);
      const auto render_frame = ogl::render::buildPlaceholderRenderFrame(scene_graph, params);

      auto logger = renderLogger();
      logger->info("Built placeholder render frame frameId={} drawItemCount={}",
                render_frame.frameId(), render_frame.drawItems().size());

      return {.success = true,
            .moduleName = module_name,
            .operationName = operation_name,
            .message = "Placeholder render frame assembled from scene graph.",
            .payload = {{"componentId", "render"},
                     {"geometryModel", geometry_model.toJson()},
                     {"sceneGraph", scene_graph.toJson()},
                     {"renderFrame", render_frame.toJson()},
                     {"summary", render_frame.summary()},
                     {"equivalentPython", buildRenderEquivalentPython(params)}}};
   }
};

class RenderServiceFactory final : public ogl::core::IServiceSingletonFactory {
public:
   auto instance() const -> tObjectSharedPtr override {
      static auto service = std::make_shared<PlaceholderRenderService>();
      return service;
   }
};

} // namespace

namespace ogl::render {

void registerRenderComponents() {
   static std::once_flag once;
   std::call_once(once, []() {
      auto logger = renderLogger();
      g_ComponentFactory.registInstanceFactoryWithID<RenderServiceFactory>("render");
      logger->info("Registered render component factory for placeholder render-frame pipeline");
   });
}

} // namespace ogl::render
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\selection\include\ogl\selection\PlaceholderSelectionResult.hpp
/**
 * @file PlaceholderSelectionResult.hpp
 * @brief Placeholder selection result types used to validate selection-layer data flow.
 */

#pragma once

#include <ogl/render/PlaceholderRenderFrame.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>
#include <ogl/selection/export.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace ogl::selection {

/**
 * @brief One placeholder hit returned by the selection layer.
 */
struct OGL_SELECTION_EXPORT PlaceholderSelectionHit {
   std::string nodeId;
   std::string displayName;
   std::string selectionType;
   int hitRank{0};

   [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Placeholder selection result built from render-frame and scene data.
 */
class OGL_SELECTION_EXPORT PlaceholderSelectionResult {
public:
   PlaceholderSelectionResult(std::string mode, std::string frame_id,
                        std::vector<PlaceholderSelectionHit> hits);

   [[nodiscard]] auto mode() const -> const std::string&;
   [[nodiscard]] auto frameId() const -> const std::string&;
   [[nodiscard]] auto hits() const -> const std::vector<PlaceholderSelectionHit>&;
   [[nodiscard]] auto summary() const -> std::string;
   [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
   std::string m_mode;
   std::string m_frameId;
   std::vector<PlaceholderSelectionHit> m_hits;
};

/**
 * @brief Evaluate a placeholder pick or box-select request from scene and render data.
 * @param scene_graph Scene-layer placeholder graph.
 * @param render_frame Render-layer placeholder frame.
 * @param params Request parameters including mode and screen-space hints.
 * @return Placeholder selection result.
 */
OGL_SELECTION_EXPORT auto evaluatePlaceholderSelection(
   const ogl::scene::PlaceholderSceneGraph& scene_graph,
   const ogl::render::PlaceholderRenderFrame& render_frame, const nlohmann::json& params)
   -> PlaceholderSelectionResult;

} // namespace ogl::selection
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\selection\include\ogl\selection\SelectionComponentRegistration.hpp
/**
 * @file SelectionComponentRegistration.hpp
 * @brief Registers placeholder selection services into Kangaroo ComponentFactory.
 */

#pragma once

#include <ogl/selection/export.hpp>

namespace ogl::selection {

/**
 * @brief Register selection services exactly once for the current process.
 */
OGL_SELECTION_EXPORT void registerSelectionComponents();

} // namespace ogl::selection
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\selection\src\PlaceholderSelectionResult.cpp
#include <ogl/selection/PlaceholderSelectionResult.hpp>

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace {

auto findDisplayName(const ogl::scene::PlaceholderSceneGraph& scene_graph, const std::string& node_id)
   -> std::string {
   for(const auto& node : scene_graph.nodes()) {
      if(node.nodeId == node_id) {
         return node.displayName;
      }
   }

   return node_id;
}

} // namespace

namespace ogl::selection {

auto PlaceholderSelectionHit::toJson() const -> nlohmann::json {
   return {{"nodeId", nodeId},
         {"displayName", displayName},
         {"selectionType", selectionType},
         {"hitRank", hitRank}};
}

PlaceholderSelectionResult::PlaceholderSelectionResult(std::string mode, std::string frame_id,
                                          std::vector<PlaceholderSelectionHit> hits)
   : m_mode(std::move(mode)), m_frameId(std::move(frame_id)), m_hits(std::move(hits)) {}

auto PlaceholderSelectionResult::mode() const -> const std::string& { return m_mode; }

auto PlaceholderSelectionResult::frameId() const -> const std::string& { return m_frameId; }

auto PlaceholderSelectionResult::hits() const -> const std::vector<PlaceholderSelectionHit>& {
   return m_hits;
}

auto PlaceholderSelectionResult::summary() const -> std::string {
   std::ostringstream stream;
   stream << "Placeholder " << m_mode << " selection resolved " << m_hits.size()
         << " hit(s) from render frame '" << m_frameId << "'.";
   return stream.str();
}

auto PlaceholderSelectionResult::toJson() const -> nlohmann::json {
   nlohmann::json hits_json = nlohmann::json::array();
   for(const auto& hit : m_hits) {
      hits_json.push_back(hit.toJson());
   }

   return {{"mode", m_mode},
         {"frameId", m_frameId},
         {"hitCount", m_hits.size()},
         {"hits", std::move(hits_json)},
         {"summary", summary()}};
}

auto evaluatePlaceholderSelection(const ogl::scene::PlaceholderSceneGraph& scene_graph,
                          const ogl::render::PlaceholderRenderFrame& render_frame,
                          const nlohmann::json& params) -> PlaceholderSelectionResult {
   const std::string mode = params.value("mode", std::string{"pick"});
   const auto& draw_items = render_frame.drawItems();
   if(draw_items.empty()) {
      return PlaceholderSelectionResult(mode, render_frame.frameId(), {});
   }

   std::size_t start_index = 0;
   std::size_t selection_count = 1;

   if(mode == "box") {
      const int requested_count = params.value("selectionCount", 2);
      selection_count = static_cast<std::size_t>(std::clamp(requested_count, 1,
                                               static_cast<int>(draw_items.size())));
      start_index = static_cast<std::size_t>(
         std::clamp(params.value("startIndex", 0), 0, static_cast<int>(draw_items.size() - 1)));
   } else {
      const int screen_x = params.value("screenX", 0);
      const int screen_y = params.value("screenY", 0);
      start_index = static_cast<std::size_t>((std::abs(screen_x) + std::abs(screen_y)) %
                                    static_cast<int>(draw_items.size()));
   }

   std::vector<PlaceholderSelectionHit> hits;
   hits.reserve(selection_count);

   for(std::size_t offset = 0; offset < selection_count; ++offset) {
      const auto& draw_item = draw_items[(start_index + offset) % draw_items.size()];
      hits.push_back({.nodeId = draw_item.nodeId,
                  .displayName = findDisplayName(scene_graph, draw_item.nodeId),
                  .selectionType = mode == "box" ? "box-select" : "pick",
                  .hitRank = static_cast<int>(offset + 1)});
   }

   return PlaceholderSelectionResult(mode, render_frame.frameId(), std::move(hits));
}

} // namespace ogl::selection
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\selection\src\SelectionComponentRegistration.cpp
#include <ogl/selection/SelectionComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/geometry/PlaceholderGeometryModel.hpp>
#include <ogl/render/PlaceholderRenderFrame.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>
#include <ogl/selection/PlaceholderSelectionResult.hpp>

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/logger_factory.hpp>

#include <mutex>
#include <sstream>

namespace {

auto selectionLogger() {
   static auto logger = Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab.Selection");
   return logger;
}

auto buildSelectionEquivalentPython(const nlohmann::json& params) -> std::string {
   std::ostringstream script;
   script << "import opengeolab\n\n";
   script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
   script << "result = bridge.call(\"selection\", R\"JSON(" << params.dump(2)
         << ")JSON\")\n";
   script << "print(result)";
   return script.str();
}

auto buildGeometryModel(const nlohmann::json& params) -> ogl::geometry::PlaceholderGeometryModel {
   return ogl::geometry::PlaceholderGeometryModel(
      {.modelName = params.value("modelName", std::string{"Bracket_A01"}),
       .bodyCount = params.value("bodyCount", 3),
       .source = params.value("source", std::string{"selection-service"})});
}

auto normalizeSelectionParams(const std::string& operation_name, const nlohmann::json& params)
   -> nlohmann::json {
   nlohmann::json normalized = params;
   if(operation_name == "boxSelectPlaceholder") {
      normalized["mode"] = "box";
      if(!normalized.contains("selectionCount")) {
         normalized["selectionCount"] = 2;
      }
   } else {
      normalized["mode"] = "pick";
   }
   return normalized;
}

class PlaceholderSelectionService final : public ogl::core::IService {
public:
   auto processRequest(const std::string& module_name, const nlohmann::json& params)
      -> ogl::core::ServiceResponse override {
      const std::string operation_name = params.value("operation", std::string{"unknown"});
      if(module_name != "selection") {
         return {.success = false,
               .moduleName = module_name,
               .operationName = operation_name,
               .message = "Placeholder selection service only accepts the selection module.",
               .payload = nlohmann::json::object()};
      }

      if(operation_name != "pickPlaceholderEntity" &&
         operation_name != "boxSelectPlaceholder") {
         return {.success = false,
               .moduleName = module_name,
               .operationName = operation_name,
               .message =
                  "Unsupported selection operation. Use pickPlaceholderEntity or boxSelectPlaceholder.",
               .payload = nlohmann::json::object()};
      }

      const auto normalized_params = normalizeSelectionParams(operation_name, params);
      const auto geometry_model = buildGeometryModel(normalized_params);
      const auto scene_graph = ogl::scene::buildPlaceholderSceneGraph(geometry_model);
      const auto render_frame = ogl::render::buildPlaceholderRenderFrame(scene_graph, normalized_params);
      const auto selection_result =
         ogl::selection::evaluatePlaceholderSelection(scene_graph, render_frame, normalized_params);

      auto logger = selectionLogger();
      logger->info("Resolved placeholder selection mode={} hitCount={}", selection_result.mode(),
                selection_result.hits().size());

      return {.success = true,
            .moduleName = module_name,
            .operationName = operation_name,
            .message = "Placeholder selection completed through geometry, scene, and render data.",
            .payload = {{"componentId", "selection"},
                     {"geometryModel", geometry_model.toJson()},
                     {"sceneGraph", scene_graph.toJson()},
                     {"renderFrame", render_frame.toJson()},
                     {"selectionResult", selection_result.toJson()},
                     {"summary", selection_result.summary()},
                     {"equivalentPython", buildSelectionEquivalentPython(normalized_params)}}};
   }
};

class SelectionServiceFactory final : public ogl::core::IServiceSingletonFactory {
public:
   auto instance() const -> tObjectSharedPtr override {
      static auto service = std::make_shared<PlaceholderSelectionService>();
      return service;
   }
};

} // namespace

namespace ogl::selection {

void registerSelectionComponents() {
   static std::once_flag once;
   std::call_once(once, []() {
      auto logger = selectionLogger();
      g_ComponentFactory.registInstanceFactoryWithID<SelectionServiceFactory>("selection");
      logger->info("Registered selection component factory for placeholder interaction pipeline");
   });
}

} // namespace ogl::selection
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\selection\tests\CMakeLists.txt
add_executable(opengeolab_selection_smoke_test test_placeholder_selection.cpp)

target_compile_features(opengeolab_selection_smoke_test PRIVATE cxx_std_20)

target_link_libraries(opengeolab_selection_smoke_test PRIVATE ogl_selection)

add_test(NAME opengeolab_selection_smoke_test COMMAND opengeolab_selection_smoke_test)
*** Add File: c:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLab\libs\selection\tests\test_placeholder_selection.cpp
#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/selection/SelectionComponentRegistration.hpp>

#include <nlohmann/json.hpp>

int main() {
   ogl::selection::registerSelectionComponents();

   const auto response = ogl::core::ComponentRequestDispatcher::dispatch(
      "selection",
      nlohmann::json{{"operation", "pickPlaceholderEntity"},
                  {"modelName", "SelectionSmokeModel"},
                  {"bodyCount", 4},
                  {"viewportWidth", 1024},
                  {"viewportHeight", 768},
                  {"screenX", 90},
                  {"screenY", 30},
                  {"source", "test"}});

   if(!response.success) {
      return 1;
   }

   const auto scene_graph = response.payload.value("sceneGraph", nlohmann::json::object());
   if(scene_graph.value("nodeCount", 0) != 4) {
      return 2;
   }

   const auto render_frame = response.payload.value("renderFrame", nlohmann::json::object());
   if(render_frame.value("drawItemCount", 0) != 4) {
      return 3;
   }

   const auto selection_result =
      response.payload.value("selectionResult", nlohmann::json::object());
   if(selection_result.value("hitCount", 0) != 1) {
      return 4;
   }

   return 0;
}