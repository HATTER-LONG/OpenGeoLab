/**
 * @file scene_renderer.hpp
 * @brief OpenGL scene renderer for geometry visualization
 *
 * Provides a self-contained OpenGL rendering component that handles
 * shader management, mesh data upload, scene rendering, and entity picking.
 * This component is independent of the QML layer and can be used by any OpenGL context.
 */

#pragma once

#include "geometry/geometry_types.hpp"
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
 * @brief Result of picking operation at a single pixel
 */
struct PickPixelResult {
    bool m_valid{false};                                           ///< Whether valid entity found
    Geometry::EntityType m_entityType{Geometry::EntityType::None}; ///< Entity type
    Geometry::EntityUID m_entityUid{0};                            ///< Entity UID
};

/**
 * @brief OpenGL scene renderer for geometry visualization
 *
 * SceneRenderer is a self-contained component that handles:
 * - Shader compilation and management
 * - Mesh data upload to GPU
 * - Scene rendering with proper lighting
 * - ID buffer rendering for entity picking
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
     * @brief Set the entity UID to highlight during rendering
     * @param uid Entity UID to highlight, or INVALID_ENTITY_UID for none
     * @note Highlighted entity will be rendered with a different color
     */
    void setHighlightedEntityUid(Geometry::EntityUID uid);

    /**
     * @brief Get the currently highlighted entity UID
     * @return Highlighted entity UID
     */
    [[nodiscard]] Geometry::EntityUID highlightedEntityUid() const;

    /**
     * @brief Render ID buffer for picking and read pixel at position
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param view_matrix View transformation matrix
     * @param projection_matrix Projection matrix
     * @return Picking result at the pixel
     */
    [[nodiscard]] PickPixelResult
    pickAtPixel(int x, int y, const QMatrix4x4& view_matrix, const QMatrix4x4& projection_matrix);

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
     * @brief Render meshes to ID buffer (for picking)
     * @param mvp Model-View-Projection matrix
     */
    void renderIdBuffer(const QMatrix4x4& mvp);
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

    /**
     * @brief Encode entity ID and type into RGBA color
     * @param entity_uid Entity UID to encode
     * @param entity_type Entity type to encode
     * @param r Output red component
     * @param g Output green component
     * @param b Output blue component
     * @param a Output alpha component
     */
    static void encodeEntityId(Geometry::EntityUID entity_uid,
                               Geometry::EntityType entity_type,
                               uint8_t& r,
                               uint8_t& g,
                               uint8_t& b,
                               uint8_t& a);

    /**
     * @brief Decode entity ID and type from RGBA color
     * @param r Red component
     * @param g Green component
     * @param b Blue component
     * @param a Alpha component
     * @param entity_uid Output entity UID
     * @param entity_type Output entity type
     * @return true if valid entity decoded
     */
    [[nodiscard]] static bool decodeEntityId(uint8_t r,
                                             uint8_t g,
                                             uint8_t b,
                                             uint8_t a,
                                             Geometry::EntityUID& entity_uid,
                                             Geometry::EntityType& entity_type);

private:
    bool m_initialized{false};
    QSize m_viewportSize{800, 600};

    // Shaders
    std::unique_ptr<QOpenGLShaderProgram> m_meshShader;
    std::unique_ptr<QOpenGLShaderProgram> m_idShader;

    // Mesh shader uniform locations
    int m_mvpMatrixLoc{-1};
    int m_modelMatrixLoc{-1};
    int m_normalMatrixLoc{-1};
    int m_lightPosLoc{-1};
    int m_viewPosLoc{-1};
    int m_pointSizeLoc{-1};
    int m_highlightedLoc{-1}; ///< Uniform location for highlight flag

    // ID shader uniform locations
    int m_idMvpMatrixLoc{-1};
    int m_idColorLoc{-1};
    int m_idPointSizeLoc{-1};
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

        Geometry::EntityUID m_entityUid{0};                            ///< Entity UID for picking
        Geometry::EntityType m_entityType{Geometry::EntityType::None}; ///< Entity type

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

    // Highlight state
    Geometry::EntityUID m_highlightedEntityUid{Geometry::INVALID_ENTITY_UID}; ///< UID to highlight
};

} // namespace OpenGeoLab::Render
