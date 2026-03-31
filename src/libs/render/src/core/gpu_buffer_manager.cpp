#include "core/gpu_buffer_manager.hpp"

#include <opengeolab/scene/scene_node.hpp>

namespace OpenGeoLab::Render {

void GpuBufferManager::initialize() {
    glGenVertexArrays(1, &m_mainVao);
    glGenVertexArrays(1, &m_pickVao);
    glGenBuffers(1, &m_mainVbo);
    glGenBuffers(1, &m_pickVbo);
    glGenBuffers(1, &m_ibo);
}

void GpuBufferManager::cleanup() {
    if(m_mainVao != 0) {
        glDeleteVertexArrays(1, &m_mainVao);
        m_mainVao = 0;
    }
    if(m_pickVao != 0) {
        glDeleteVertexArrays(1, &m_pickVao);
        m_pickVao = 0;
    }
    if(m_mainVbo != 0) {
        glDeleteBuffers(1, &m_mainVbo);
        m_mainVbo = 0;
    }
    if(m_pickVbo != 0) {
        glDeleteBuffers(1, &m_pickVbo);
        m_pickVbo = 0;
    }
    if(m_ibo != 0) {
        glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
    }

    m_uploadedVersion = 0;
    m_triangleRanges.clear();
    m_lineRanges.clear();
    m_pointRanges.clear();
    m_hasData = false;
}

void GpuBufferManager::synchronize(const Scene::SceneGraph& scene) {
    const uint64_t scene_ver = scene.version();
    if(scene_ver == m_uploadedVersion) {
        return;
    }

    rebuildBuffers(scene);
    m_uploadedVersion = scene_ver;
}

void GpuBufferManager::rebuildBuffers(const Scene::SceneGraph& scene) {
    std::vector<Scene::RenderVertex> all_vertices;
    std::vector<Scene::PickIdEntry> all_pick_ids;
    std::vector<uint32_t> all_indices;

    m_triangleRanges.clear();
    m_lineRanges.clear();
    m_pointRanges.clear();

    scene.traverseVisible([&](const Scene::SceneNode& node) {
        const Scene::IRenderComponent* const render_component = node.renderComponent();
        if(render_component == nullptr) {
            return;
        }

        const auto& mesh_data = render_component->meshData();
        if(mesh_data.vertices.empty()) {
            return;
        }

        const auto vertex_base = static_cast<uint32_t>(all_vertices.size());
        const auto index_base = static_cast<uint32_t>(all_indices.size());

        all_vertices.insert(all_vertices.end(), mesh_data.vertices.begin(),
                            mesh_data.vertices.end());
        all_pick_ids.insert(all_pick_ids.end(), mesh_data.pickIds.begin(), mesh_data.pickIds.end());

        for(const uint32_t index : mesh_data.indices) {
            all_indices.push_back(index + vertex_base);
        }

        const auto adjust_and_append = [&](const std::vector<Scene::DrawRange>& source,
                                           std::vector<Scene::DrawRange>& destination) {
            for(auto range : source) {
                range.vertexOffset += vertex_base;
                range.indexOffset += index_base;
                destination.push_back(range);
            }
        };

        adjust_and_append(mesh_data.triangleRanges, m_triangleRanges);
        adjust_and_append(mesh_data.lineRanges, m_lineRanges);
        adjust_and_append(mesh_data.pointRanges, m_pointRanges);
    });

    m_hasData = !all_vertices.empty();
    if(!m_hasData) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_mainVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(all_vertices.size() * sizeof(Scene::RenderVertex)),
                 all_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_pickVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(all_pick_ids.size() * sizeof(Scene::PickIdEntry)),
                 all_pick_ids.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(all_indices.size() * sizeof(uint32_t)), all_indices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    setupMainVao();
    setupPickVao();
}

void GpuBufferManager::setupMainVao() {
    constexpr GLsizei k_main_stride = sizeof(Scene::RenderVertex);

    glBindVertexArray(m_mainVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_mainVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, k_main_stride,
                          reinterpret_cast<const void*>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, k_main_stride,
                          reinterpret_cast<const void*>(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, k_main_stride,
                          reinterpret_cast<const void*>(6 * sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBindVertexArray(0);
}

void GpuBufferManager::setupPickVao() {
    constexpr GLsizei k_main_stride = sizeof(Scene::RenderVertex);
    constexpr GLsizei k_pick_stride = sizeof(Scene::PickIdEntry);

    glBindVertexArray(m_pickVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_mainVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, k_main_stride,
                          reinterpret_cast<const void*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, m_pickVbo);
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, k_pick_stride, reinterpret_cast<const void*>(0));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBindVertexArray(0);
}

void GpuBufferManager::bindMainVao() const { glBindVertexArray(m_mainVao); }

void GpuBufferManager::bindPickVao() const { glBindVertexArray(m_pickVao); }

void GpuBufferManager::unbind() const { glBindVertexArray(0); }

const std::vector<Scene::DrawRange>& GpuBufferManager::triangleRanges() const noexcept {
    return m_triangleRanges;
}

const std::vector<Scene::DrawRange>& GpuBufferManager::lineRanges() const noexcept {
    return m_lineRanges;
}

const std::vector<Scene::DrawRange>& GpuBufferManager::pointRanges() const noexcept {
    return m_pointRanges;
}

bool GpuBufferManager::hasData() const noexcept { return m_hasData; }

} // namespace OpenGeoLab::Render
