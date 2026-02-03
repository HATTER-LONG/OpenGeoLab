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
     * @brief Render the complete scene
     * @param camera_pos Camera position for lighting
     * @param view_matrix View transformation matrix
     * @param projection_matrix Projection matrix
     */
    void render(const QVector3D& camera_pos,
                const QMatrix4x4& view_matrix,
                const QMatrix4x4& projection_matrix);

    /**
     * @brief Render the scene for entity picking (color-coded entity IDs)
     * @param view_matrix View transformation matrix
     * @param projection_matrix Projection matrix
     * @param selection_mode Selection mode filter (which entity types to render)
     */
    void renderForPicking(const QMatrix4x4& view_matrix,
                          const QMatrix4x4& projection_matrix,
                          Geometry::SelectionMode selection_mode);

    /**
     * @brief Decode an entity ID from a picked color
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     * @param a Alpha component (0-255)
     * @return Decoded EntityId, or INVALID_ENTITY_ID if background
     */
    [[nodiscard]] static Geometry::EntityId
    decodeEntityIdFromColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

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
    void renderMeshesForPicking(const QMatrix4x4& mvp, Geometry::SelectionMode selection_mode);

    /**
     * @brief Encode an entity ID to RGBA color for picking
     * @param entity_id Entity ID to encode
     * @return QVector4D with RGBA values (normalized 0-1)
     */
    [[nodiscard]] static QVector4D encodeEntityIdToColor(Geometry::EntityId entity_id);

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
    std::unique_ptr<QOpenGLShaderProgram> m_pickingShader; ///< Shader for entity ID picking

    // Mesh shader uniform locations
    int m_mvpMatrixLoc{-1};
    int m_modelMatrixLoc{-1};
    int m_normalMatrixLoc{-1};
    int m_lightPosLoc{-1};
    int m_viewPosLoc{-1};
    int m_pointSizeLoc{-1};
    int m_highlightStateLoc{-1}; ///< Uniform for highlight state (0=None, 1=Preview, 2=Selected)
    int m_highlightColorLoc{-1}; ///< Uniform for highlight color override

    // Picking shader uniform locations
    int m_pickingMvpLoc{-1};       ///< MVP matrix for picking shader
    int m_pickingColorLoc{-1};     ///< Entity color for picking shader
    int m_pickingPointSizeLoc{-1}; ///< Point size for picking shader

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
        HighlightState m_highlightState{HighlightState::None}; ///< Highlight state for rendering
        Geometry::EntityId m_entityId{Geometry::INVALID_ENTITY_ID}; ///< Entity ID for picking

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
};

} // namespace OpenGeoLab::Render
