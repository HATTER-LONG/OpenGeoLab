/**
 * @file lighting.cpp
 * @brief Implementation of lighting system for 3D rendering
 */

#include "render/lighting.hpp"

namespace OpenGeoLab {
namespace Render {

Light Light::directional(const QVector3D& direction, const QVector3D& color, float intensity) {
    Light light;
    light.type = LightType::Directional;
    light.position = direction.normalized();
    light.color = color;
    light.intensity = intensity;
    return light;
}

Light Light::point(const QVector3D& position, const QVector3D& color, float intensity) {
    Light light;
    light.type = LightType::Point;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    return light;
}

Light Light::headlight(const QVector3D& color, float intensity) {
    Light light;
    light.type = LightType::Headlight;
    light.color = color;
    light.intensity = intensity;
    return light;
}

Material Material::solidColor(const QColor& color) {
    Material mat;
    mat.diffuse = QVector3D(color.redF(), color.greenF(), color.blueF());
    mat.useVertexColors = false;
    return mat;
}

LightingEnvironment::LightingEnvironment() { setupDefaultLighting(); }

int LightingEnvironment::addLight(const Light& light) {
    m_lights.push_back(light);
    return static_cast<int>(m_lights.size() - 1);
}

void LightingEnvironment::removeLight(int index) {
    if(index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights.erase(m_lights.begin() + index);
    }
}

void LightingEnvironment::clear() { m_lights.clear(); }

void LightingEnvironment::setupDefaultLighting() {
    clear();

    // Three-point lighting setup
    // Key light (main light from upper-right-front)
    addLight(Light::directional(QVector3D(1.0f, 1.0f, 1.0f), QVector3D(1.0f, 0.98f, 0.95f), 0.8f));

    // Fill light (softer light from left)
    addLight(Light::directional(QVector3D(-1.0f, 0.5f, 0.5f), QVector3D(0.95f, 0.95f, 1.0f), 0.4f));

    // Back/rim light
    addLight(Light::directional(QVector3D(0.0f, -0.5f, -1.0f), QVector3D(1.0f, 1.0f, 1.0f), 0.3f));

    m_ambientIntensity = 0.15f;
}

void LightingEnvironment::setupCADLighting() {
    clear();

    // CAD-style lighting with headlight and fill
    addLight(Light::headlight(QVector3D(1.0f, 1.0f, 1.0f), 0.6f));

    // Upper directional for better edge definition
    addLight(Light::directional(QVector3D(0.0f, 1.0f, 0.3f), QVector3D(1.0f, 1.0f, 1.0f), 0.5f));

    // Side fill
    addLight(Light::directional(QVector3D(1.0f, 0.0f, 0.0f), QVector3D(0.9f, 0.9f, 1.0f), 0.3f));

    m_ambientIntensity = 0.2f;
}

} // namespace Render
} // namespace OpenGeoLab
