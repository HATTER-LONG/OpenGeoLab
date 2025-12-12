/**
 * @file lighting.cpp
 * @brief Implementation of lighting system
 */

#include <render/lighting.hpp>

namespace OpenGeoLab {
namespace Rendering {

LightingEnvironment::LightingEnvironment() { setupDefaultLighting(); }

int LightingEnvironment::addLight(const Light& light) {
    m_lights.push_back(light);
    return static_cast<int>(m_lights.size()) - 1;
}

void LightingEnvironment::removeLight(int index) {
    if(index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights.erase(m_lights.begin() + index);
    }
}

void LightingEnvironment::clear() { m_lights.clear(); }

void LightingEnvironment::setupDefaultLighting() {
    clear();

    // Three-point lighting setup for natural look

    // Key light - main light source from upper right front
    addLight(Light::directional(QVector3D(1.0f, 1.0f, 1.0f).normalized(),
                                QVector3D(1.0f, 0.98f, 0.95f), // Slightly warm
                                0.8f));

    // Fill light - softer light from left to fill shadows
    addLight(Light::directional(QVector3D(-0.8f, 0.5f, 0.5f).normalized(),
                                QVector3D(0.9f, 0.95f, 1.0f), // Slightly cool
                                0.4f));

    // Rim/Back light - light from behind to create edge definition
    addLight(Light::directional(QVector3D(0.0f, 0.3f, -1.0f).normalized(),
                                QVector3D(1.0f, 1.0f, 1.0f), 0.3f));

    // Ambient light
    m_ambientColor = QVector3D(1.0f, 1.0f, 1.0f);
    m_ambientIntensity = 0.2f;
}

void LightingEnvironment::setupCADLighting() {
    clear();

    // CAD-style lighting optimized for technical visualization
    // Uses more uniform lighting to show all surfaces clearly

    // Main light from upper front
    addLight(Light::directional(QVector3D(0.0f, 1.0f, 1.0f).normalized(),
                                QVector3D(1.0f, 1.0f, 1.0f), 0.6f));

    // Side light from right
    addLight(Light::directional(QVector3D(1.0f, 0.5f, 0.5f).normalized(),
                                QVector3D(1.0f, 1.0f, 1.0f), 0.5f));

    // Side light from left
    addLight(Light::directional(QVector3D(-1.0f, 0.5f, 0.5f).normalized(),
                                QVector3D(1.0f, 1.0f, 1.0f), 0.4f));

    // Bottom fill light
    addLight(Light::directional(QVector3D(0.0f, -0.5f, 0.5f).normalized(),
                                QVector3D(1.0f, 1.0f, 1.0f), 0.2f));

    // Higher ambient for CAD visualization
    m_ambientColor = QVector3D(1.0f, 1.0f, 1.0f);
    m_ambientIntensity = 0.25f;
}

} // namespace Rendering
} // namespace OpenGeoLab
