/**
 * @file scene_renderer.hpp
 * @brief OpenGL scene renderer for geometry visualization
 *
 * Provides a self-contained OpenGL rendering component that handles
 * shader management, mesh data upload, and scene rendering. This component
 * is independent of the QML layer and can be used by any OpenGL context.
 */

#pragma once

#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QSize>
#include <QVector3D>
#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief OpenGL scene renderer for geometry visualization
 *
 * SceneRenderer is a self-contained component that handles:
 * - Shader compilation and management
 * - Mesh data upload to GPU
 * - Scene rendering with proper lighting
 *
 * @note Must be used within a valid OpenGL context.
 * @note Thread safety: Not thread-safe, all calls must be from the render thread.
 */
class SceneRenderer : protected QOpenGLFunctions {
public:
    SceneRenderer();
    ~SceneRenderer();

    // Non-copyable, non-movable (OpenGL resources cannot be moved)
    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;
    SceneRenderer(SceneRenderer&&) = delete;
    SceneRenderer& operator=(SceneRenderer&&) = delete;

    /**
     * @brief Initialize OpenGL resources
     * @note Must be called from the OpenGL context thread after context is current
     */
    void initialize();

    /**
     * @brief Check if renderer is initialized
     * @return true if initialize() has been called successfully
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief Update viewport size for aspect ratio calculations
     * @param size New viewport size in pixels
     */
    void setViewportSize(const QSize& size);

    /**
     * @brief Upload mesh data to GPU
     * @param render_data Document render data containing meshes to upload
     * @note Call this when render data changes
     */
    void uploadMeshData(const DocumentRenderData& render_data);

    /**
     * @brief Render a picking pass encoding (uid+type) into the framebuffer color.
     *
     * Caller should bind the desired framebuffer before calling, and use glReadPixels
     * to read back the pixel under the cursor.
     */
    void renderPicking(const QMatrix4x4& view_matrix, const QMatrix4x4& projection_matrix);

    /**
     * @brief Highlight a specific entity (matched by type + uid).
     * @note Pass EntityType::None or INVALID_ENTITY_UID to clear highlight.
     */
    void setHighlightedEntity(Geometry::EntityType type, Geometry::EntityUID uid);

    /**
     * @brief Render the complete scene
     * @param camera_pos Camera position for lighting
     * @param view_matrix View transformation matrix
     * @param projection_matrix Projection matrix
     */
    void render(const QVector3D& camera_pos,
                const QMatrix4x4& view_matrix,
                const QMatrix4x4& projection_matrix);

    /**
     * @brief Cleanup GPU resources
     * @note Call before destroying OpenGL context
     */
    void cleanup();

private:
    void setupShaders();
    void renderMeshes(const QMatrix4x4& mvp,
                      const QMatrix4x4& model_matrix,
                      const QVector3D& camera_pos);

    /**
     * @brief Upload a single mesh to GPU buffers
     * @param mesh Mesh data to upload
     * @param vao VAO to bind
     * @param vbo VBO to use
     * @param ebo EBO to use (optional)
     */
    void uploadMesh(const RenderMesh& mesh,
                    QOpenGLVertexArrayObject& vao,
                    QOpenGLBuffer& vbo,
                    QOpenGLBuffer& ebo);

    /**
     * @brief Clear mesh buffers and release GPU resources
     */
    void clearMeshBuffers();

private:
    bool m_initialized{false};
    QSize m_viewportSize{800, 600};

    // Shaders
    std::unique_ptr<QOpenGLShaderProgram> m_meshShader;
    std::unique_ptr<QOpenGLShaderProgram> m_pickShader;
    std::unique_ptr<QOpenGLShaderProgram> m_pickEdgeShader;

    // Mesh shader uniform locations
    int m_mvpMatrixLoc{-1};
    int m_modelMatrixLoc{-1};
    int m_normalMatrixLoc{-1};
    int m_lightPosLoc{-1};
    int m_viewPosLoc{-1};
    int m_pointSizeLoc{-1};
    int m_useLightingLoc{-1};

    int m_useOverrideColorLoc{-1};
    int m_overrideColorLoc{-1};

    int m_pickMvpMatrixLoc{-1};
    int m_pickColorLoc{-1};
    int m_pickPointSizeLoc{-1};

    int m_pickEdgeMvpMatrixLoc{-1};
    int m_pickEdgeColorLoc{-1};
    int m_pickEdgeViewportLoc{-1};
    int m_pickEdgeThicknessLoc{-1};

    /**
     * @brief Internal structure for mesh GPU buffers
     */
    struct MeshBuffers {
        std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
        std::unique_ptr<QOpenGLBuffer> m_vbo;
        std::unique_ptr<QOpenGLBuffer> m_ebo;
        int m_vertexCount{0};
        int m_indexCount{0};
        RenderPrimitiveType m_primitiveType{RenderPrimitiveType::Triangles};

        Geometry::EntityType m_entityType{Geometry::EntityType::None};
        Geometry::EntityUID m_entityUid{Geometry::INVALID_ENTITY_UID};

        MeshBuffers();
        ~MeshBuffers();

        MeshBuffers(MeshBuffers&&) noexcept = default;
        MeshBuffers& operator=(MeshBuffers&&) noexcept = default;

        MeshBuffers(const MeshBuffers&) = delete;
        MeshBuffers& operator=(const MeshBuffers&) = delete;

        void destroy();
    };

    std::vector<MeshBuffers> m_faceMeshBuffers;
    std::vector<MeshBuffers> m_edgeMeshBuffers;
    std::vector<MeshBuffers> m_vertexMeshBuffers;

    Geometry::EntityType m_highlightType{Geometry::EntityType::None};
    Geometry::EntityUID m_highlightUid{Geometry::INVALID_ENTITY_UID};
};

} // namespace OpenGeoLab::Render
