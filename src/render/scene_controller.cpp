/**
 * @file scene_controller.cpp
 * @brief Implementation of SceneController for managing 3D scene interactions
 */

#include <render/scene_controller.hpp>

#include <core/logger.hpp>

namespace OpenGeoLab {
namespace Rendering {

SceneController::SceneController(QObject* parent) : QObject(parent) {}

void SceneController::setRenderer(OpenGLRenderer* renderer) {
    m_renderer = renderer;
    LOG_DEBUG("SceneController: Renderer set");
}

void SceneController::setGeometryData(std::shared_ptr<Geometry::GeometryData> geometry_data) {
    if(!m_renderer || !geometry_data) {
        LOG_WARN("SceneController: Cannot set geometry - renderer or data is null");
        return;
    }

    m_renderer->setGeometryData(geometry_data);
    LOG_INFO("SceneController: Geometry set with {} vertices, {} indices",
             geometry_data->vertexCount(), geometry_data->indexCount());

    // Calculate bounding box
    float min_point[3], max_point[3];
    if(geometry_data->getBoundingBox(min_point, max_point)) {
        m_hasBounds = true;
        m_boundsMin = QVector3D(min_point[0], min_point[1], min_point[2]);
        m_boundsMax = QVector3D(max_point[0], max_point[1], max_point[2]);

        // Set model center for proper rotation pivot
        QVector3D center = (m_boundsMin + m_boundsMax) * 0.5f;
        m_renderer->setModelCenter(center);

        LOG_INFO("SceneController: Bounds calculated - min({}, {}, {}), max({}, {}, {})",
                 m_boundsMin.x(), m_boundsMin.y(), m_boundsMin.z(), m_boundsMax.x(),
                 m_boundsMax.y(), m_boundsMax.z());
    } else {
        m_hasBounds = false;
        LOG_WARN("SceneController: Could not calculate bounds");
    }

    // Reset model rotation for new geometry
    resetModelRotation();

    emit geometryChanged();
}

void SceneController::rotateModel(float delta_yaw, float delta_pitch) {
    if(m_renderer) {
        m_renderer->rotateModel(delta_yaw, delta_pitch);
        emit viewChanged();
    }
}

void SceneController::resetModelRotation() {
    if(m_renderer) {
        m_renderer->resetModelRotation();
        emit viewChanged();
    }
}

void SceneController::zoom(float factor) {
    if(m_renderer && m_renderer->camera()) {
        m_renderer->camera()->zoom(factor);
        emit viewChanged();
    }
}

void SceneController::pan(float delta_x, float delta_y) {
    if(m_renderer && m_renderer->camera()) {
        m_renderer->camera()->pan(delta_x, delta_y);
        emit viewChanged();
    }
}

void SceneController::fitToView(float margin) {
    if(!m_renderer || !m_renderer->camera()) {
        LOG_WARN("SceneController: Cannot fit to view - renderer or camera not available");
        return;
    }

    if(m_hasBounds) {
        m_renderer->camera()->fitToBounds(m_boundsMin, m_boundsMax, margin);
        LOG_INFO("SceneController: Fit to view completed");
    } else {
        m_renderer->camera()->reset();
        LOG_DEBUG("SceneController: No bounds, reset camera to default");
    }

    emit viewChanged();
}

void SceneController::resetView() {
    if(m_renderer && m_renderer->camera()) {
        m_renderer->camera()->reset();
        resetModelRotation();
        LOG_DEBUG("SceneController: View reset to default");
        emit viewChanged();
    }
}

void SceneController::setViewFront() {
    if(m_renderer) {
        m_renderer->setModelRotation(0.0f, 0.0f);
        LOG_DEBUG("SceneController: Set view to front");
        emit viewChanged();
    }
}

void SceneController::setViewBack() {
    if(m_renderer) {
        m_renderer->setModelRotation(180.0f, 0.0f);
        LOG_DEBUG("SceneController: Set view to back");
        emit viewChanged();
    }
}

void SceneController::setViewTop() {
    if(m_renderer) {
        m_renderer->setModelRotation(0.0f, -90.0f);
        LOG_DEBUG("SceneController: Set view to top");
        emit viewChanged();
    }
}

void SceneController::setViewBottom() {
    if(m_renderer) {
        m_renderer->setModelRotation(0.0f, 90.0f);
        LOG_DEBUG("SceneController: Set view to bottom");
        emit viewChanged();
    }
}

void SceneController::setViewLeft() {
    if(m_renderer) {
        m_renderer->setModelRotation(90.0f, 0.0f);
        LOG_DEBUG("SceneController: Set view to left");
        emit viewChanged();
    }
}

void SceneController::setViewRight() {
    if(m_renderer) {
        m_renderer->setModelRotation(-90.0f, 0.0f);
        LOG_DEBUG("SceneController: Set view to right");
        emit viewChanged();
    }
}

void SceneController::setViewIsometric() {
    if(m_renderer) {
        // Standard isometric: 45 degrees yaw, ~35.264 degrees pitch
        m_renderer->setModelRotation(-45.0f, -35.264f);
        LOG_DEBUG("SceneController: Set view to isometric");
        emit viewChanged();
    }
}

} // namespace Rendering
} // namespace OpenGeoLab
