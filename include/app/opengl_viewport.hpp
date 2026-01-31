/**
 * @file gl_viewport.hpp
 * @brief QQuickFramebufferObject-based OpenGL viewport for geometry rendering
 *
 * GLViewport provides a QML-integrable 3D viewport that renders geometry
 * using OpenGL. It supports camera manipulation (rotate, pan, zoom) and
 * integrates with RenderService for scene management.
 */

#pragma once

#include "app/render_service.hpp"
#include "render/render_data.hpp"


#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QQuickFramebufferObject>
#include <QtQml/qqml.h>
#include <memory>

namespace OpenGeoLab::App {

class GLViewportRenderer;

/**
 * @brief QML-integrable OpenGL viewport for 3D geometry visualization
 *
 * GLViewport provides:
 * - OpenGL rendering of geometry from RenderService
 * - Mouse-based camera manipulation (orbit, pan, zoom)
 * - Integration with the application's render service
 */
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Render::RenderService* renderService READ renderService WRITE setRenderService NOTIFY
                   renderServiceChanged)

public:
    explicit GLViewport(QQuickItem* parent = nullptr);
    ~GLViewport() override;

    /**
     * @brief Create the renderer for this viewport
     * @return New renderer instance
     */
    Renderer* createRenderer() const override;

    /**
     * @brief Get the associated render service
     * @return Pointer to the render service
     */
    [[nodiscard]] Render::RenderService* renderService() const;

    /**
     * @brief Set the render service
     * @param service Render service to use
     */
    void setRenderService(Render::RenderService* service);

    /**
     * @brief Get current camera state for rendering
     * @return Camera state reference
     */
    [[nodiscard]] const Render::CameraState& cameraState() const;

    /**
     * @brief Get render data for rendering
     * @return Document render data reference
     */
    [[nodiscard]] const Render::DocumentRenderData& renderData() const;

signals:
    void renderServiceChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onSceneNeedsUpdate();

private:
    /**
     * @brief Perform orbit camera rotation
     * @param dx Horizontal mouse delta
     * @param dy Vertical mouse delta
     */
    void orbitCamera(float dx, float dy);

    /**
     * @brief Perform camera panning
     * @param dx Horizontal mouse delta
     * @param dy Vertical mouse delta
     */
    void panCamera(float dx, float dy);

    /**
     * @brief Perform camera zoom
     * @param delta Zoom amount
     */
    void zoomCamera(float delta);

private:
    Render::RenderService* m_renderService{nullptr}; ///< Associated render service
    Render::CameraState m_cameraState;               ///< Local camera state copy

    QPointF m_lastMousePos;            ///< Last mouse position for delta calculation
    Qt::MouseButtons m_pressedButtons; ///< Currently pressed mouse buttons
};

/**
 * @brief OpenGL renderer for GLViewport
 *
 * Handles the actual OpenGL rendering of geometry meshes.
 * Created by GLViewport::createRenderer().
 */
class GLViewportRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    explicit GLViewportRenderer(const GLViewport* viewport);
    ~GLViewportRenderer() override;

    /**
     * @brief Create the framebuffer object
     * @param size Framebuffer size
     * @return New framebuffer object
     */
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

    /**
     * @brief Render the scene
     */
    void render() override;

    /**
     * @brief Synchronize with the viewport item
     * @param item The viewport item
     */
    void synchronize(QQuickFramebufferObject* item) override;

private:
    void initializeGL();
    void setupShaders();
    void uploadMeshData();
    void renderMeshes();
    void renderGrid();

    /**
     * @brief Upload a single mesh to GPU
     * @param mesh Mesh data to upload
     * @param vao VAO to bind
     * @param vbo VBO to use
     * @param ebo EBO to use (optional)
     */
    void uploadMesh(const Render::RenderMesh& mesh,
                    QOpenGLVertexArrayObject& vao,
                    QOpenGLBuffer& vbo,
                    QOpenGLBuffer& ebo);

private:
    const GLViewport* m_viewport{nullptr}; ///< Parent viewport item

    bool m_initialized{false};     ///< OpenGL initialization state
    bool m_needsDataUpload{false}; ///< Whether mesh data needs uploading

    std::unique_ptr<QOpenGLShaderProgram> m_shaderProgram; ///< Main shader program
    std::unique_ptr<QOpenGLShaderProgram> m_gridShader;    ///< Grid shader program

    // Shader uniform locations
    int m_mvpMatrixLoc{-1};
    int m_modelMatrixLoc{-1};
    int m_normalMatrixLoc{-1};
    int m_lightPosLoc{-1};
    int m_viewPosLoc{-1};
    int m_pointSizeLoc{-1};

    // Grid rendering
    QOpenGLVertexArrayObject m_gridVAO;
    QOpenGLBuffer m_gridVBO;
    int m_gridVertexCount{0};

    // Mesh rendering buffers
    struct MeshBuffers {
        std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
        std::unique_ptr<QOpenGLBuffer> m_vbo;
        std::unique_ptr<QOpenGLBuffer> m_ebo;
        int m_vertexCount{0};
        int m_indexCount{0};
        Render::RenderPrimitiveType m_primitiveType{Render::RenderPrimitiveType::Triangles};

        MeshBuffers()
            : m_vao(std::make_unique<QOpenGLVertexArrayObject>()),
              m_vbo(std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer)),
              m_ebo(std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer)) {}
        ~MeshBuffers() = default;

        MeshBuffers(MeshBuffers&&) noexcept = default;
        MeshBuffers& operator=(MeshBuffers&&) noexcept = default;

        MeshBuffers(const MeshBuffers&) = delete;
        MeshBuffers& operator=(const MeshBuffers&) = delete;
    };

    std::vector<MeshBuffers> m_faceMeshBuffers;   ///< Face mesh GPU buffers
    std::vector<MeshBuffers> m_edgeMeshBuffers;   ///< Edge mesh GPU buffers
    std::vector<MeshBuffers> m_vertexMeshBuffers; ///< Vertex mesh GPU buffers

    // Cached render data
    Render::DocumentRenderData m_renderData;
    Render::CameraState m_cameraState;
    QSize m_viewportSize;
};

} // namespace OpenGeoLab::App
