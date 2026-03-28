#pragma once
#include "engine/components/Camera.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace systems {

class FreeFlyCamera {
public:
    explicit FreeFlyCamera(GLFWwindow* window, components::Camera& camera,
                           int toggleKey = GLFW_KEY_O);

    bool update(float dt);

    [[nodiscard]] bool isActive() const { return m_active; }

private:
    void activate();
    void deactivate();
    void processKeyboard(float dt);
    void processMouse();

    GLFWwindow*          m_window;
    components::Camera&  m_camera;
    int                  m_toggleKey;

    bool  m_active       = false;
    bool  m_toggleHeld   = false;
    bool  m_escHeld      = false;

    float m_yaw          = -90.0f;
    float m_pitch        = 0.0f;
    float m_speed        = 8.0f;
    float m_fastMult     = 3.0f;
    float m_sensitivity  = 0.12f;

    double m_lastCursorX = 0.0;
    double m_lastCursorY = 0.0;
    bool   m_firstMouse  = true;

    components::Camera m_savedCamera;
};

} // namespace systems
