#include "game/systems/InputSystem.h"
#include "game/config/GameConfig.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

using namespace config;

InputSystem::InputSystem(GLFWwindow* window, const components::Camera& camera)
    : m_window(window), m_camera(camera) {}

InputSystem::Cell InputSystem::mouseToCell() const {
    double mx, my;
    glfwGetCursorPos(m_window, &mx, &my);

    int winW, winH;
    glfwGetWindowSize(m_window, &winW, &winH);
    if (winW == 0 || winH == 0) return {-1, -1};

    float ndcX = (static_cast<float>(mx) / winW) * 2.0f - 1.0f;
    float ndcY = (static_cast<float>(my) / winH) * 2.0f - 1.0f;

    float aspect = static_cast<float>(winW) / static_cast<float>(winH);
    glm::mat4 invVP = glm::inverse(m_camera.projection(aspect) * m_camera.view());
    glm::vec4 near4 = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 far4  = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    glm::vec3 nearPt = glm::vec3(near4) / near4.w;
    glm::vec3 farPt  = glm::vec3(far4)  / far4.w;

    float tZ = -nearPt.z / (farPt.z - nearPt.z);
    glm::vec3 hit = nearPt + tZ * (farPt - nearPt);

    int col = static_cast<int>(std::round(hit.x / CELL_SIZE + BOARD_W * 0.5f - 0.5f));
    int row = static_cast<int>(std::round(hit.y / CELL_SIZE + BOARD_H * 0.5f - 0.5f));

    if (col < 0 || col >= BOARD_W || row < 0 || row >= BOARD_H)
        return {-1, -1};
    return {col, row};
}

void InputSystem::clearSelection() {
    m_selCol = m_selRow = -1;
    m_dragging = false;
    m_dragCommitted = false;
}

InputResult InputSystem::update(int& outCursorCol, int& outCursorRow) {
    InputResult result;
    outCursorCol = m_selCol;
    outCursorRow = m_selRow;

    bool mouseDown    = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool justPressed  = mouseDown && !m_lastMouseDown;
    bool justReleased = !mouseDown && m_lastMouseDown;
    m_lastMouseDown   = mouseDown;

    auto [col, row] = mouseToCell();

    if (justPressed && col >= 0) {
        m_dragging      = true;
        m_dragCol       = col;
        m_dragRow       = row;
        m_dragCommitted = false;

        if (m_selCol >= 0) {
            int dc = std::abs(col - m_selCol);
            int dr = std::abs(row - m_selRow);
            if ((dc == 1 && dr == 0) || (dc == 0 && dr == 1)) {
                result.wantSwap = true;
                result.fromCol  = m_selCol;
                result.fromRow  = m_selRow;
                result.toCol    = col;
                result.toRow    = row;
                m_dragging = false;
                m_selCol = m_selRow = -1;
                outCursorCol = outCursorRow = -1;
                return result;
            }
        }

        m_selCol = col;
        m_selRow = row;
        outCursorCol = col;
        outCursorRow = row;
    }

    if (m_dragging && mouseDown && !m_dragCommitted && col >= 0) {
        int dc = std::abs(col - m_dragCol);
        int dr = std::abs(row - m_dragRow);
        if ((dc == 1 && dr == 0) || (dc == 0 && dr == 1)) {
            result.wantSwap = true;
            result.fromCol  = m_dragCol;
            result.fromRow  = m_dragRow;
            result.toCol    = col;
            result.toRow    = row;
            m_dragCommitted = true;
            m_dragging = false;
            m_selCol = m_selRow = -1;
            outCursorCol = outCursorRow = -1;
            return result;
        }
    }

    if (justReleased) {
        m_dragging      = false;
        m_dragCommitted = false;
    }

    return result;
}
