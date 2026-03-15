#include <ogl/geometry/GeometryModel.hpp>

#include <algorithm>
#include <utility>

namespace OGL::Geometry {

GeometryModel::GeometryModel(GeometryDescriptor descriptor)
    : m_descriptor(std::move(descriptor)) {
    if(m_descriptor.modelName.empty()) {
        m_descriptor.modelName = "GeometryModel";
    }
    if(m_descriptor.source.empty()) {
        m_descriptor.source = "component";
    }
    m_descriptor.bodyCount = std::max(m_descriptor.bodyCount, 1);
}

auto GeometryModel::modelName() const -> const std::string& {
    return m_descriptor.modelName;
}

auto GeometryModel::bodyCount() const -> int { return m_descriptor.bodyCount; }

auto GeometryModel::source() const -> const std::string& { return m_descriptor.source; }

auto GeometryModel::summary() const -> std::string {
    return "Geometry model '" + m_descriptor.modelName + "' contains " +
           std::to_string(m_descriptor.bodyCount) + " conceptual bodies from " +
           m_descriptor.source + ".";
}

auto GeometryModel::toJson() const -> nlohmann::json {
    return {
        {"modelName", m_descriptor.modelName},
        {"bodyCount", m_descriptor.bodyCount},
        {"source", m_descriptor.source},
        {"summary", summary()},
    };
}

} // namespace OGL::Geometry

