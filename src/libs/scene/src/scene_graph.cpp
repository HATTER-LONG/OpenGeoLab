/**
 * @file scene_graph.cpp
 * @brief Implements SceneGraph traversal and mutation utilities.
 */
#include <opengeolab/scene/scene_graph.hpp>

#include <ranges>
#include <utility>

namespace OpenGeoLab::Scene {

namespace {

SceneNode* findNodeById(SceneNode& node, const int id) {
    if(node.id == id) {
        return &node;
    }

    for(SceneNode& child : node.children) {
        if(SceneNode* const found = findNodeById(child, id); found != nullptr) {
            return found;
        }
    }

    return nullptr;
}

const SceneNode* findNodeById(const SceneNode& node, const int id) {
    if(node.id == id) {
        return &node;
    }

    for(const SceneNode& child : node.children) {
        if(const SceneNode* const found = findNodeById(child, id); found != nullptr) {
            return found;
        }
    }

    return nullptr;
}

bool removeNodeRecursive(SceneNode& parent, const int id) {
    if(const auto child_it =
           std::ranges::find(parent.children, id, [](const SceneNode& child) { return child.id; });
       child_it != parent.children.end()) {
        parent.children.erase(child_it);
        return true;
    }

    for(SceneNode& child : parent.children) {
        if(removeNodeRecursive(child, id)) {
            return true;
        }
    }

    return false;
}

void mergeWorldBounds(const SceneNode& node, BoundingBox& bounds) {
    bounds.merge(node.bounds);

    for(const SceneNode& child : node.children) {
        mergeWorldBounds(child, bounds);
    }
}

} // namespace

SceneGraph::SceneGraph() {
    root_.id = 0;
    root_.name = "root";
}

SceneNode& SceneGraph::root() { return root_; }

const SceneNode& SceneGraph::root() const { return root_; }

SceneNode* SceneGraph::findById(const int id) { return findNodeById(root_, id); }

const SceneNode* SceneGraph::findById(const int id) const { return findNodeById(root_, id); }

int SceneGraph::addNode(SceneNode node, const int parentId) {
    SceneNode* const parent = findById(parentId);
    if(parent == nullptr) {
        return 0;
    }

    node.id = nextId_++;
    parent->children.push_back(std::move(node));

    if(onChanged) {
        onChanged();
    }

    return parent->children.back().id;
}

bool SceneGraph::removeNode(const int id) {
    if(id == root_.id) {
        return false;
    }

    if(!removeNodeRecursive(root_, id)) {
        return false;
    }

    if(onChanged) {
        onChanged();
    }

    return true;
}

BoundingBox SceneGraph::worldBounds() const {
    BoundingBox bounds;
    mergeWorldBounds(root_, bounds);
    return bounds;
}

} // namespace OpenGeoLab::Scene
