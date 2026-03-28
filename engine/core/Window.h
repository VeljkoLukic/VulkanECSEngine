#pragma once
#include <GLFW/glfw3.h>
#include <functional>
#include <stdexcept>

namespace core {

class Window {
public:
    Window(int width, int height, const char* title) {
        if (!glfwInit())
            throw std::runtime_error("Failed to initialise GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE,  GLFW_TRUE);

        m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!m_window)
            throw std::runtime_error("Failed to create GLFW window");

        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, onFramebufferResize);
    }

    ~Window() {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]] GLFWwindow* handle()      const { return m_window; }
    [[nodiscard]] bool        shouldClose() const { return glfwWindowShouldClose(m_window); }
    [[nodiscard]] bool        wasResized()  const { return m_resized; }

    void pollEvents()       { glfwPollEvents(); }
    void clearResizedFlag() { m_resized = false; }

    void setResizeCallback(std::function<void(int, int)> cb) {
        m_resizeCb = std::move(cb);
    }

private:
    static void onFramebufferResize(GLFWwindow* win, int w, int h) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
        self->m_resized = true;
        if (self->m_resizeCb) self->m_resizeCb(w, h);
    }

    GLFWwindow*                  m_window   = nullptr;
    bool                         m_resized  = false;
    std::function<void(int,int)> m_resizeCb;
};

} // namespace core
