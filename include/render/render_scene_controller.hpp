#pragma once

#include "geometry/geometry_document.hpp"
#include "geometry/geometry_types.hpp"
#include "render/render_data.hpp"

#include <QMatrix4x4>
#include <QVector3D>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Render {
/**
 * @brief Camera configuration for a 3D viewport
 */
struct CameraState {
    QVector3D m_position{0.0f, 0.0f, 50.0f}; ///< Camera position in world space
    QVector3D m_target{0.0f, 0.0f, 0.0f};    ///< Look-at target point
    QVector3D m_up{0.0f, 1.0f, 0.0f};        ///< Up vector
    float m_fov{45.0f};                      ///< Field of view in degrees
    float m_nearPlane{0.1f};                 ///< Near clipping plane
    float m_farPlane{10000.0f};              ///< Far clipping plane

    /**
     * @brief Update clipping planes based on the current camera distance
     * @param distance Distance between camera position and target
     */
    void updateClipping(float distance);

    /**
     * @brief Build a view matrix
     * @return View transformation matrix
     */
    [[nodiscard]] QMatrix4x4 viewMatrix() const;

    /**
     * @brief Build a perspective projection matrix
     * @param aspect_ratio Viewport aspect ratio (width/height)
     * @return Perspective projection matrix
     */
    [[nodiscard]] QMatrix4x4 projectionMatrix(float aspect_ratio) const;

    /**
     * @brief Reset camera to default configuration
     */
    void reset();

    /**
     * @brief Fit camera to view a bounding box
     * @param bbox Target bounding box
     */
    void fitToBoundingBox(const Geometry::BoundingBox3D& bbox);
};

enum class SceneUpdateType : uint8_t {
    GeometryChanged = 0,
    MeshChanged = 1,
    CameraChanged = 2,
};

class RenderSceneController : public Kangaroo::Util::NonCopyMoveable {
protected:
    RenderSceneController();

public:
    static RenderSceneController& instance();
    virtual ~RenderSceneController();

    virtual CameraState& cameraState() { return m_cameraState; }

    void setCamera(const CameraState& camera, bool notify = true);

    void refreshScene(bool notify = true);

    void fitToScene(bool notify = true);

    void resetCamera(bool notify = true);

    void setFrontView(bool notify = true);

    void setBackView(bool notify = true);

    void setTopView(bool notify = true);

    void setBottomView(bool notify = true);

    void setLeftView(bool notify = true);

    void setRightView(bool notify = true);

    [[nodiscard]] const RenderData& renderData() const;

    [[nodiscard]] Geometry::GeometryDocumentPtr currentGeometryDocument() const;

    void setPartGeometryVisible(Geometry::EntityUID part_uid, bool visible);

    void setPartMeshVisible(Geometry::EntityUID part_uid, bool visible);
    [[nodiscard]] bool isPartGeometryVisible(Geometry::EntityUID part_uid) const;
    [[nodiscard]] bool isPartMeshVisible(Geometry::EntityUID part_uid) const;

    [[nodiscard]] Util::ScopedConnection
    subscribeToSceneNeedsUpdate(std::function<void(SceneUpdateType)> callback) {
        return m_sceneNeedsUpdate.connect(callback);
    }

private:
    void subscribeToGeometryDocument();

    void subscribeToMeshDocument();

    void handleDocumentGeometryChanged(const Geometry::GeometryChangeEvent& event);

    void handleDocumentMeshChanged();

    void updateGeometryRenderData();

    void updateMeshRenderData();

private:
    CameraState m_cameraState;

    Util::ScopedConnection m_geometryDocumentConnection;
    Util::ScopedConnection m_meshDocumentConnection;

    Util::Signal<SceneUpdateType> m_sceneNeedsUpdate;

    RenderData m_renderData;

    struct PartVisibility {
        bool m_geometryVisible{true};
        bool m_meshVisible{true};
    };
    mutable std::mutex m_visibilityMutex;
    std::unordered_map<Geometry::EntityUID, PartVisibility> m_partVisibility;
};
} // namespace OpenGeoLab::Render