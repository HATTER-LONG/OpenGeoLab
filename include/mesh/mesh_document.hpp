#pragma once
#include "mesh/mesh_element.hpp"
#include "mesh/mesh_node.hpp"
#include <kangaroo/util/noncopyable.hpp>
#include <memory>

namespace OpenGeoLab::Mesh {
class MeshDocument;
using MeshDocumentPtr = std::shared_ptr<MeshDocument>;

class MeshDocument : public Kangaroo::Util::NonCopyable {
public:
    MeshDocument() = default;
    virtual ~MeshDocument() = default;

    // -------------------------------------------------------------------------
    // Node Management
    // -------------------------------------------------------------------------
    virtual bool addNode(const MeshNode& node) = 0;

    [[nodiscard]] virtual MeshNode& findNodeByID(MeshNodeId node_id) = 0;

    [[nodiscard]] virtual size_t getNodeCount() const = 0;

    // -------------------------------------------------------------------------
    // Element Management
    // -------------------------------------------------------------------------
    virtual bool addElement(const MeshElement& element) = 0;

    [[nodiscard]] virtual MeshElement& findElementByID(MeshElementId element_id) = 0;

    [[nodiscard]] virtual MeshElement& findElementByUID(MeshElementUID element_uid,
                                                        MeshElementType element_type) = 0;

    [[nodiscard]] virtual size_t getElementCount() const = 0;
};
} // namespace OpenGeoLab::Mesh