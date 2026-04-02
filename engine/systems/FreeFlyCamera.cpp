#include "engine/systems/FreeFlyCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

namespace systems {

FreeFlyCamera::FreeFlyCamera(GLFWwindow* window, components::Camera& camera, int toggleKey)
    : m_window(window)
    , m_camera(camera)
    , m_toggleKey(toggleKey)
{}

bool FreeFlyCamera::update(float dt)
{
    bool toggleDown = glfwGetKey(m_window, m_toggleKey) == GLFW_PRESS;
    bool toggleJust = toggleDown && !m_toggleHeld;
    m_toggleHeld = toggleDown;

    bool escDown = glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool escJust = escDown && !m_escHeld;
    m_escHeld = escDown;

    if (toggleJust) {
        if (!m_active) activate();
        else           deactivate();
    }
    if (escJust && m_active) {
        deactivate();
    }

    if (!m_active) return false;

    processKeyboard(dt);
    processMouse();

    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front   = glm::normalize(front);

    m_camera.target = m_camera.eye + front;
    m_camera.up     = { 0.0f, 1.0f, 0.0f };

    return true;
}

void FreeFlyCamera::activate()
{
    m_savedCamera = m_camera;
    m_active      = true;
    m_firstMouse  = true;

    glm::vec3 dir = glm::normalize(m_camera.target - m_camera.eye);
    m_yaw   = glm::degrees(std::atan2(dir.z, dir.x));
    m_pitch = glm::degrees(std::asin(std::clamp(dir.y, -1.0f, 1.0f)));

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void FreeFlyCamera::deactivate()
{
    m_active = false;
    m_camera = m_savedCamera;
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void FreeFlyCamera::processKeyboard(float dt)
{
    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front   = glm::normalize(front);

    glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };
    glm::vec3 right   = glm::normalize(glm::cross(front, worldUp));
    //glm::vec3 up      = glm::normalize(glm::cross(right, front));

    float speed = m_speed * dt;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
        speed *= m_fastMult;

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)  m_camera.eye += front * speed;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)  m_camera.eye -= front * speed;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)  m_camera.eye -= right * speed;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)  m_camera.eye += right * speed;
    //if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS)  m_camera.eye += up    * speed;
    //if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)  m_camera.eye -= up    * speed;
}

void FreeFlyCamera::processMouse()
{
    double cx, cy;
    glfwGetCursorPos(m_window, &cx, &cy);

    if (m_firstMouse) {
        m_lastCursorX = cx;
        m_lastCursorY = cy;
        m_firstMouse  = false;
        return;
    }

    float dx = static_cast<float>(cx - m_lastCursorX);
    float dy = static_cast<float>(m_lastCursorY - cy);
    m_lastCursorX = cx;
    m_lastCursorY = cy;

    m_yaw   += dx * m_sensitivity;
    m_pitch += dy * m_sensitivity;
    m_pitch  = std::clamp(m_pitch, -89.0f, 89.0f);
}

}
