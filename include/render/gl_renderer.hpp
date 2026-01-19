/**
 * @file gl_renderer.hpp
 * @brief OpenGL renderer for 3D geometry visualization
 *
 * Provides GPU-accelerated rendering of tessellated geometry with
 * support for selection highlighting and picking operations.
 */

#pragma once

#include "render/render_data.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief OpenGL-based renderer for 3D geometry
 *
 * Manages GPU resources and renders tessellated geometry using
 * modern OpenGL with VBOs and shaders.
 */
class GLRenderer : protected QOpenGLFunctions {
public:
    GLRenderer();
    ~GLRenderer();

    /**
     * @brief Initialize OpenGL resources
     * @return true if initialization succeeded
     * @note Must be called with a valid OpenGL context active
     */
    bool initialize();

    /**
     * @brief Release all OpenGL resources
     * @note Must be called with a valid OpenGL context active
     */
    void cleanup();

    /**
     * @brief Check if renderer is initialized
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief Set the render scene to display
     */
    void setScene(const RenderScene& scene);

    /**
     * @brief Update GPU buffers for a single mesh
     */
    void updateMesh(const RenderMeshPtr& mesh);

    /**
     * @brief Remove a mesh from GPU memory
     */
    void removeMesh(Geometry::EntityId entityId);

    /**
     * @brief Clear all GPU resources for meshes
     */
    void clearMeshes();

    /**
     * @brief Set the camera for rendering
     */
    void setCamera(const Camera& camera);

    /**
     * @brief Set display settings
     */
    void setDisplaySettings(const DisplaySettings& settings);

    /**
     * @brief Render the current scene
     * @param width Viewport width in pixels
     * @param height Viewport height in pixels
     */
    void render(int width, int height);

    /**
     * @brief Perform picking at screen coordinates
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param width Viewport width
     * @param height Viewport height
     * @return Pick result with hit information
     */
    PickResult pick(int x, int y, int width, int height);

    /**
     * @brief Set the highlighted entity (hover state)
     */
    void setHighlightedEntity(Geometry::EntityId entityId);

    /**
     * @brief Set selected entities
     */
    void setSelectedEntities(const std::vector<Geometry::EntityId>& entityIds);

private:
    /**
     * @brief GPU buffer data for a single mesh
     */
    struct MeshBuffers {
        Geometry::EntityId entityId{Geometry::INVALID_ENTITY_ID};
        QOpenGLVertexArrayObject faceVao;
        QOpenGLBuffer faceVbo{QOpenGLBuffer::VertexBuffer};
        QOpenGLBuffer faceIbo{QOpenGLBuffer::IndexBuffer};
        int faceIndexCount{0};

        QOpenGLVertexArrayObject edgeVao;
        QOpenGLBuffer edgeVbo{QOpenGLBuffer::VertexBuffer};
        QOpenGLBuffer edgeIbo{QOpenGLBuffer::IndexBuffer};
        int edgeIndexCount{0};

        bool visible{true};
        bool selected{false};
        bool highlighted{false};
        Geometry::Color baseColor;
    };

    bool compileShaders();
    void setupVertexAttributes();
    void uploadMeshToGPU(const RenderMeshPtr& mesh, MeshBuffers& buffers);
    void renderMeshFaces(MeshBuffers& buffers);
    void renderMeshEdges(MeshBuffers& buffers);
    void updateViewProjection(int width, int height);

    MeshBuffers* findMeshBuffers(Geometry::EntityId entityId);

private:
    bool m_initialized{false};

    std::unique_ptr<QOpenGLShaderProgram> m_faceShader;
    std::unique_ptr<QOpenGLShaderProgram> m_edgeShader;
    std::unique_ptr<QOpenGLShaderProgram> m_pickShader;

    std::vector<std::unique_ptr<MeshBuffers>> m_meshBuffers;

    Camera m_camera;
    DisplaySettings m_displaySettings;
    Light m_mainLight;

    Geometry::EntityId m_highlightedEntity{Geometry::INVALID_ENTITY_ID};
    std::vector<Geometry::EntityId> m_selectedEntities;

    // Uniform locations
    int m_faceModelViewLoc{-1};
    int m_faceProjectionLoc{-1};
    int m_faceNormalMatrixLoc{-1};
    int m_faceLightDirLoc{-1};
    int m_faceLightColorLoc{-1};
    int m_faceSelectedLoc{-1};
    int m_faceHighlightedLoc{-1};
    int m_faceSelectedColorLoc{-1};
    int m_faceHighlightColorLoc{-1};

    int m_edgeModelViewLoc{-1};
    int m_edgeProjectionLoc{-1};
};

} // namespace OpenGeoLab::Render
