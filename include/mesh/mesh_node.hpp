#include "mesh/mesh_types.hpp"
#include "util/point_vector3d.hpp"
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Mesh {
class MeshNode {
public:
    MeshNode(double x, double y, double z);
    ~MeshNode() = default;

    double x() const { return m_position.x; }
    double y() const { return m_position.y; }
    double z() const { return m_position.z; }

    const Util::Pt3d& position() const { return m_position; }

    void setPosition(double x, double y, double z) { m_position = Util::Pt3d{x, y, z}; }
    void setPosition(const Util::Pt3d& pos) { m_position = pos; }

private:
    MeshNodeId m_uid{INVALID_MESH_NODE_ID};
    Util::Pt3d m_position;
};
} // namespace OpenGeoLab::Mesh