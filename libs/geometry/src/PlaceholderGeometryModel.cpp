#include <ogl/geometry/PlaceholderGeometryModel.hpp>

#include <algorithm>
#include <utility>

namespace OGL::Geometry {

PlaceholderGeometryModel::PlaceholderGeometryModel(PlaceholderGeometryDescriptor descriptor)
    : m_descriptor(std::move(descriptor)) {
    if(m_descriptor.modelName.empty()) {
        m_descriptor.modelName = "PlaceholderGeometry";
    }
    if(m_descriptor.source.empty()) {
        m_descriptor.source = "component";
    }
    m_descriptor.bodyCount = std::max(m_descriptor.bodyCount, 1);
}

auto PlaceholderGeometryModel::modelName() const -> const std::string& {
    return m_descriptor.modelName;
}

auto PlaceholderGeometryModel::bodyCount() const -> int { return m_descriptor.bodyCount; }

auto PlaceholderGeometryModel::source() const -> const std::string& { return m_descriptor.source; }

auto PlaceholderGeometryModel::summary() const -> std::string {
    return "Placeholder geometry model '" + m_descriptor.modelName + "' contains " +
           std::to_string(m_descriptor.bodyCount) +
           " conceptual bodies and entered the pipeline from " + m_descriptor.source + ".";
}

auto PlaceholderGeometryModel::toJson() const -> nlohmann::json {
    return {
        {"modelName", m_descriptor.modelName},
        {"bodyCount", m_descriptor.bodyCount},
        {"source", m_descriptor.source},
        {"summary", summary()},
    };
}

} // namespace OGL::Geometry