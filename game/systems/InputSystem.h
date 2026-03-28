#pragma once
#include "engine/components/Camera.h"
#include "game/board/Board.h"
#include <GLFW/glfw3.h>

struct InputResult {
    bool  wantSwap = false;
    int   fromCol  = -1;
    int   fromRow  = -1;
    int   toCol    = -1;
    int   toRow    = -1;
};

class InputSystem {
public:
    InputSystem(GLFWwindow* window, const components::Camera& camera);

    InputResult update(int& outCursorCol, int& outCursorRow);
    void clearSelection();

private:
    struct Cell { int col, row; };
    Cell mouseToCell() const;

    GLFWwindow*               m_window;
    const components::Camera& m_camera;

    int  m_selCol = -1, m_selRow = -1;

    bool m_lastMouseDown = false;
    bool m_dragging      = false;
    int  m_dragCol = -1, m_dragRow = -1;
    bool m_dragCommitted = false;
};
