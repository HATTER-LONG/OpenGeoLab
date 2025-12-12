/**
 * @file lighting.hpp
 * @brief Lighting system for 3D rendering
 *
 * Provides a flexible lighting configuration including:
 * - Multiple light sources (directional, point)
 * - Ambient, diffuse, and specular components
 * - Material properties
 */

#pragma once

#include <QColor>
#include <QVector3D>
#include <QVector4D>
#include <vector>

namespace OpenGeoLab {
namespace Rendering {

/**
 * @brief Light source types
 */
enum class LightType {
    Directional, // Infinite distance light (like sun)
    Point,       // Point light with attenuation
    Headlight    // Light attached to camera
};

/**
 * @brief Single light source configuration
 */
struct Light {
    LightType type = LightType::Directional;
    QVector3D position = QVector3D(1, 1, 1); // Position or direction
    QVector3D color = QVector3D(1, 1, 1);    // Light color
    float intensity = 1.0f;                  // Light intensity
    float constant = 1.0f;                   // Attenuation constant (point lights)
    float linear = 0.09f;                    // Attenuation linear (point lights)
    float quadratic = 0.032f;                // Attenuation quadratic (point lights)

    /**
     * @brief Create directional light
     */
    static Light directional(const QVector3D& direction,
                             const QVector3D& color = QVector3D(1, 1, 1),
                             float intensity = 1.0f) {
        Light light;
        light.type = LightType::Directional;
        light.position = direction.normalized();
        light.color = color;
        light.intensity = intensity;
        return light;
    }

    /**
     * @brief Create point light
     */
    static Light point(const QVector3D& position,
                       const QVector3D& color = QVector3D(1, 1, 1),
                       float intensity = 1.0f) {
        Light light;
        light.type = LightType::Point;
        light.position = position;
        light.color = color;
        light.intensity = intensity;
        return light;
    }

    /**
     * @brief Create headlight (follows camera)
     */
    static Light headlight(const QVector3D& color = QVector3D(1, 1, 1), float intensity = 0.5f) {
        Light light;
        light.type = LightType::Headlight;
        light.color = color;
        light.intensity = intensity;
        return light;
    }
};

/**
 * @brief Material properties for shading
 */
struct Material {
    QVector3D ambient = QVector3D(0.2f, 0.2f, 0.2f);  // Ambient color
    QVector3D diffuse = QVector3D(0.8f, 0.8f, 0.8f);  // Diffuse color
    QVector3D specular = QVector3D(0.5f, 0.5f, 0.5f); // Specular color
    float shininess = 32.0f;                          // Specular shininess
    bool useVertexColors = true;                      // Use per-vertex colors for diffuse

    /**
     * @brief Create default material
     */
    static Material defaultMaterial() { return Material(); }

    /**
     * @brief Create material with uniform color
     */
    static Material solidColor(const QColor& color) {
        Material mat;
        mat.diffuse = QVector3D(color.redF(), color.greenF(), color.blueF());
        mat.useVertexColors = false;
        return mat;
    }
};

/**
 * @brief Lighting environment configuration
 *
 * Manages all lights and global lighting settings for a scene.
 */
class LightingEnvironment {
public:
    LightingEnvironment();

    /**
     * @brief Add a light source
     * @param light Light to add
     * @return Index of the added light
     */
    int addLight(const Light& light);

    /**
     * @brief Remove a light by index
     * @param index Light index to remove
     */
    void removeLight(int index);

    /**
     * @brief Get all lights
     * @return Vector of lights
     */
    const std::vector<Light>& lights() const { return m_lights; }

    /**
     * @brief Get light by index
     * @param index Light index
     * @return Light reference
     */
    Light& light(int index) { return m_lights[index]; }

    /**
     * @brief Get number of lights
     * @return Light count
     */
    int lightCount() const { return static_cast<int>(m_lights.size()); }

    /**
     * @brief Set ambient light color
     * @param color Ambient color
     */
    void setAmbientColor(const QVector3D& color) { m_ambientColor = color; }

    /**
     * @brief Get ambient light color
     * @return Ambient color
     */
    QVector3D ambientColor() const { return m_ambientColor; }

    /**
     * @brief Set ambient light intensity
     * @param intensity Ambient intensity
     */
    void setAmbientIntensity(float intensity) { m_ambientIntensity = intensity; }

    /**
     * @brief Get ambient light intensity
     * @return Ambient intensity
     */
    float ambientIntensity() const { return m_ambientIntensity; }

    /**
     * @brief Clear all lights
     */
    void clear();

    /**
     * @brief Setup default three-point lighting
     */
    void setupDefaultLighting();

    /**
     * @brief Setup CAD-style lighting (good for technical visualization)
     */
    void setupCADLighting();

private:
    std::vector<Light> m_lights;
    QVector3D m_ambientColor = QVector3D(1, 1, 1);
    float m_ambientIntensity = 0.15f;
};

} // namespace Rendering
} // namespace OpenGeoLab
