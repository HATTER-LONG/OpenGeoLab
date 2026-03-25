/**
 * @file geometry_module.cpp
 * @brief OCC-backed geometry module implementation.
 */

#include <opengeolab/geometry/geometry_module.hpp>

#include "occ_primitives.hpp"
#include "shape_store.hpp"
#include "tessellator.hpp"

#include <opengeolab/base/notification_registry.hpp>
#include <opengeolab/base/notification_sink.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/scene_node.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <exception>
#include <utility>

namespace OpenGeoLab::Geometry {

struct GeometryModule::Impl {
    Scene::SceneGraph& graph;
    ShapeStore store;

    explicit Impl(Scene::SceneGraph& graph_ref) : graph(graph_ref) {}
};

GeometryModule::GeometryModule(Scene::SceneGraph& graph) : impl_(std::make_unique<Impl>(graph)) {}

GeometryModule::~GeometryModule() = default;
GeometryModule::GeometryModule(GeometryModule&&) noexcept = default;
GeometryModule& GeometryModule::operator=(GeometryModule&&) noexcept = default;

namespace {

void notifyIfAvailable(std::string_view channel, std::string_view payload) {
    if(auto* sink = Base::NotificationRegistry::sink()) {
        sink->notify(channel, payload);
    }
}

nlohmann::json handleCreatePrimitive(Scene::SceneGraph& graph,
                                     ShapeStore& store,
                                     const std::string& action,
                                     const nlohmann::json& req,
                                     const ModuleProgressCallback& progress) {
    TopoDS_Shape shape;
    std::string label;

    if(action == "create_box") {
        const auto center = req.value("center", std::array<double, 3>{0.0, 0.0, 0.0});
        const auto size = req.value("size", std::array<double, 3>{1.0, 1.0, 1.0});
        shape = makeBox(center, size);
        label = req.value("label", std::string("Box"));
    } else if(action == "create_cylinder") {
        const auto center = req.value("center", std::array<double, 3>{0.0, 0.0, 0.0});
        const double radius = req.value("radius", 1.0);
        const double height = req.value("height", 1.0);
        shape = makeCylinder(center, radius, height);
        label = req.value("label", std::string("Cylinder"));
    } else if(action == "create_sphere") {
        const auto center = req.value("center", std::array<double, 3>{0.0, 0.0, 0.0});
        const double radius = req.value("radius", 1.0);
        shape = makeSphere(center, radius);
        label = req.value("label", std::string("Sphere"));
    } else if(action == "create_torus") {
        const auto center = req.value("center", std::array<double, 3>{0.0, 0.0, 0.0});
        const double majorRadius = req.value("majorRadius", 2.0);
        const double minorRadius = req.value("minorRadius", 0.5);
        shape = makeTorus(center, majorRadius, minorRadius);
        label = req.value("label", std::string("Torus"));
    }

    if(progress) {
        progress(0.3, "Tessellating...");
    }

    auto faceMesh = Tessellator::tessellate(shape);
    auto edgeMesh = Tessellator::extractEdges(shape);
    const auto bounds = Tessellator::computeBounds(shape);

    if(progress) {
        progress(0.7, "Storing shape...");
    }

    const int shapeId = store.addShape(shape, label, faceMesh, edgeMesh, bounds);

    Scene::SceneNode node;
    node.name = label;
    node.type = Scene::EntityType::Body;
    node.meshes = {faceMesh, edgeMesh};
    node.bounds = bounds;

    const int nodeId = graph.addNode(std::move(node));
    store.setSceneNodeId(shapeId, nodeId);

    const nlohmann::json changePayload = {{"event", "data_changed"}, {"count", store.shapeCount()}};
    notifyIfAvailable("geometry.data_changed", changePayload.dump());

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"id", shapeId}, {"label", label}};
}

nlohmann::json handleListShapes(ShapeStore& store) {
    const auto infos = store.allInfos();
    auto shapes = nlohmann::json::array();
    for(const auto& info : infos) {
        shapes.push_back(
            {{"id", info.id}, {"label", info.label}, {"sceneNodeId", info.sceneNodeId}});
    }
    return {{"count", static_cast<int>(infos.size())}, {"shapes", std::move(shapes)}};
}

} // namespace

std::string GeometryModule::process(std::string_view requestJson,
                                    const ModuleProgressCallback& progress) {
    auto req = nlohmann::json::parse(requestJson, nullptr, false);
    if(req.is_discarded()) {
        return R"({"ok":false,"error":"invalid JSON"})";
    }

    const auto action = req.value("action", std::string{});
    if(action.empty()) {
        return R"({"ok":false,"error":"missing action"})";
    }

    try {
        nlohmann::json result;
        if(action == "create_box" || action == "create_cylinder" || action == "create_sphere" ||
           action == "create_torus") {
            result = handleCreatePrimitive(impl_->graph, impl_->store, action, req, progress);
        } else if(action == "list_shapes") {
            result = handleListShapes(impl_->store);
        } else {
            return nlohmann::json{{"ok", false}, {"error", "unknown action: " + action}}.dump();
        }

        return nlohmann::json{{"ok", true}, {"result", std::move(result)}}.dump();
    } catch(const std::exception& exception) {
        return nlohmann::json{{"ok", false}, {"error", exception.what()}}.dump();
    }
}

} // namespace OpenGeoLab::Geometry
