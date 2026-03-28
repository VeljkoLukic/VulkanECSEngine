#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace components {

struct Camera {
    glm::vec3 eye    = { 0.0f, 0.0f, 5.0f };
    glm::vec3 target = { 0.0f, 0.0f, 0.0f };
    glm::vec3 up     = { 0.0f, 1.0f, 0.0f };

    float fovY  = 45.0f;
    float zNear = 0.1f;
    float zFar  = 100.0f;

    [[nodiscard]] glm::mat4 view() const {
        return glm::lookAt(eye, target, up);
    }

    [[nodiscard]] glm::mat4 projection(float aspect) const {
        glm::mat4 proj = glm::perspective(glm::radians(fovY), aspect, zNear, zFar);
        proj[1][1] *= -1.0f;
        return proj;
    }
};

} // namespace components
