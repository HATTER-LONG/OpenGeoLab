#include <opengeolab/scene/transform.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace OpenGeoLab::Scene {

glm::mat4 Transform::matrix() const {
    glm::mat4 t = glm::translate(glm::mat4{1.f}, m_position);
    glm::mat4 r = glm::mat4_cast(m_rotation);
    glm::mat4 s = glm::scale(glm::mat4{1.f}, m_scale);
    return t * r * s;
}

const glm::vec3& Transform::position() const { return m_position; }

const glm::quat& Transform::rotation() const { return m_rotation; }

const glm::vec3& Transform::scale() const { return m_scale; }

void Transform::setPosition(const glm::vec3& pos) { m_position = pos; }

void Transform::setRotation(const glm::quat& rot) { m_rotation = rot; }

void Transform::setScale(const glm::vec3& s) { m_scale = s; }

void Transform::reset() {
    m_position = glm::vec3{0.f};
    m_rotation = glm::quat{1.f, 0.f, 0.f, 0.f};
    m_scale = glm::vec3{1.f};
}

} // namespace OpenGeoLab::Scene
