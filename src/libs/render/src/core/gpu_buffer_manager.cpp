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
    const uint64_t sceneVer = scene.version();
    if(sceneVer == m_uploadedVersion) {
        return;
    }

    rebuildBuffers(scene);
    m_uploadedVersion = sceneVer;
}

void GpuBufferManager::rebuildBuffers(const Scene::SceneGraph& scene) {
    std::vector<Scene::RenderVertex> allVertices;
    std::vector<Scene::PickIdEntry> allPickIds;
    std::vector<uint32_t> allIndices;

    m_triangleRanges.clear();
    m_lineRanges.clear();
    m_pointRanges.clear();

    scene.traverseVisible([&](const Scene::SceneNode& node) {
        const Scene::IRenderComponent* const renderComponent = node.renderComponent();
        if(renderComponent == nullptr) {
            return;
        }

        const auto& meshData = renderComponent->meshData();
        if(meshData.vertices.empty()) {
            return;
        }

        const auto vertexBase = static_cast<uint32_t>(allVertices.size());
        const auto indexBase = static_cast<uint32_t>(allIndices.size());

        allVertices.insert(allVertices.end(), meshData.vertices.begin(), meshData.vertices.end());
        allPickIds.insert(allPickIds.end(), meshData.pickIds.begin(), meshData.pickIds.end());

        for(const uint32_t index : meshData.indices) {
            allIndices.push_back(index + vertexBase);
        }

        const auto adjustAndAppend = [&](const std::vector<Scene::DrawRange>& source,
                                         std::vector<Scene::DrawRange>& destination) {
            for(auto range : source) {
                range.vertexOffset += vertexBase;
                range.indexOffset += indexBase;
                destination.push_back(range);
            }
        };

        adjustAndAppend(meshData.triangleRanges, m_triangleRanges);
        adjustAndAppend(meshData.lineRanges, m_lineRanges);
        adjustAndAppend(meshData.pointRanges, m_pointRanges);
    });

    m_hasData = !allVertices.empty();
    if(!m_hasData) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_mainVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(allVertices.size() * sizeof(Scene::RenderVertex)),
                 allVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_pickVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(allPickIds.size() * sizeof(Scene::PickIdEntry)),
                 allPickIds.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(allIndices.size() * sizeof(uint32_t)), allIndices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    setupMainVao();
    setupPickVao();
}

void GpuBufferManager::setupMainVao() {
    constexpr GLsizei kMainStride = sizeof(Scene::RenderVertex);

    glBindVertexArray(m_mainVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_mainVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kMainStride, reinterpret_cast<const void*>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kMainStride,
                          reinterpret_cast<const void*>(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, kMainStride,
                          reinterpret_cast<const void*>(6 * sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBindVertexArray(0);
}

void GpuBufferManager::setupPickVao() {
    constexpr GLsizei kMainStride = sizeof(Scene::RenderVertex);
    constexpr GLsizei kPickStride = sizeof(Scene::PickIdEntry);

    glBindVertexArray(m_pickVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_mainVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kMainStride, reinterpret_cast<const void*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, m_pickVbo);
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, kPickStride, reinterpret_cast<const void*>(0));

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
