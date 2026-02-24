#include "geometry/geometry_types.hpp"
#include "mesh/mesh_types.hpp"

namespace OpenGeoLab::Render {
enum class RenderEntityType : uint8_t {
    Vertex = 0,
    Edge = 1,
    Wire = 2,
    Face = 3,
    Shell = 4,
    Solid = 5,
    CompSolid = 6,
    Compound = 7,
    Part = 8,

    MeshNode = 9,
    MeshLine = 10,
    MeshTriangle = 11,
    MeshQuad4 = 12,
    MeshTetra4 = 13,
    MeshHexa8 = 14,
    MeshPrism6 = 15,
    MeshPyramid5 = 16,

    None = 17,
};
enum class RenderEntityTypeMask : uint32_t {
    None = 0,

    Vertex = 1 << 0,
    Edge = 1 << 1,
    Wire = 1 << 2,
    Face = 1 << 3,
    Shell = 1 << 4,
    Solid = 1 << 5,
    CompSolid = 1 << 6,
    Compound = 1 << 7,
    Part = 1 << 8,

    MeshNode = 1 << 9,
    MeshLine = 1 << 10,
    MeshTriangle = 1 << 11,
    MeshQuad4 = 1 << 12,
    MeshTetra4 = 1 << 13,
    MeshHexa8 = 1 << 14,
    MeshPrism6 = 1 << 15,
    MeshPyramid5 = 1 << 16,
};

constexpr RenderEntityTypeMask operator|(RenderEntityTypeMask a, RenderEntityTypeMask b) {
    return static_cast<RenderEntityTypeMask>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr RenderEntityTypeMask operator&(RenderEntityTypeMask a, RenderEntityTypeMask b) {
    return static_cast<RenderEntityTypeMask>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr auto RENDER_MESH_ELEMENTS =
    RenderEntityTypeMask::MeshTriangle | RenderEntityTypeMask::MeshQuad4 |
    RenderEntityTypeMask::MeshTetra4 | RenderEntityTypeMask::MeshHexa8 |
    RenderEntityTypeMask::MeshPrism6 | RenderEntityTypeMask::MeshPyramid5;

constexpr RenderEntityTypeMask toMask(RenderEntityType type) {
    return static_cast<RenderEntityTypeMask>(1 << static_cast<uint8_t>(type));
}

constexpr RenderEntityType toRenderEntityType(Geometry::EntityType t) {
    if(t == Geometry::EntityType::None) {
        return RenderEntityType::None;
    }
    return static_cast<RenderEntityType>(static_cast<uint8_t>(t));
}

constexpr RenderEntityType toRenderEntityType(Mesh::MeshElementType t) {
    if(t == Mesh::MeshElementType::None) {
        return RenderEntityType::None;
    }
    return static_cast<RenderEntityType>(static_cast<uint8_t>(t) + 9);
}

constexpr bool isGeometryDomain(RenderEntityType t) noexcept {
    const auto val = static_cast<uint8_t>(t);
    return val >= 0 && val <= 8;
}

constexpr bool isMeshDomain(RenderEntityType t) noexcept {
    const auto val = static_cast<uint8_t>(t);
    return val >= 9 && val <= 16;
}

constexpr Geometry::EntityType toGeometryType(RenderEntityType t) {
    if(isGeometryDomain(t)) {
        return static_cast<Geometry::EntityType>(static_cast<uint8_t>(t));
    }
    return Geometry::EntityType::None;
}

constexpr Mesh::MeshElementType toMeshElementType(RenderEntityType t) {
    if(isMeshDomain(t)) {
        return static_cast<Mesh::MeshElementType>(static_cast<uint8_t>(t) - 9);
    }
    return Mesh::MeshElementType::None;
}

} // namespace OpenGeoLab::Render