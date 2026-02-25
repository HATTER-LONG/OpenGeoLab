#pragma once
#include "render/render_data.hpp"

#include <kangaroo/util/noncopyable.hpp>

#include <atomic>
#include <cstdint>

namespace OpenGeoLab::Util {

enum class ColorTheme : uint8_t {
    Light = 0,
    Dark = 1,
};

class ColorMap : public Kangaroo::Util::NonCopyMoveable {
    ColorMap() = default;

public:
    static const ColorMap& instance();
    static ColorMap& mutableInstance();
    ~ColorMap() = default;

    void setTheme(ColorTheme theme) noexcept;
    void setThemeMode(int mode) noexcept;
    [[nodiscard]] ColorTheme theme() const noexcept;

    const Render::RenderColor& getColorForPartId(Geometry::EntityUID part_uid) const noexcept;
    const Render::RenderColor&
    getColorForMeshElementId(Mesh::MeshElementUID element_uid) const noexcept;

    const Render::RenderColor& getHoverColor() const noexcept;
    const Render::RenderColor& getSelectionColor() const noexcept;

    const Render::RenderColor& getEdgeColor() const noexcept;
    const Render::RenderColor& getVertexColor() const noexcept;
    const Render::RenderColor& getMeshNodeColor() const noexcept;
    const Render::RenderColor& getMeshLineColor() const noexcept;

private:
    std::atomic<uint8_t> m_themeIndex{static_cast<uint8_t>(ColorTheme::Light)};
};
} // namespace OpenGeoLab::Util