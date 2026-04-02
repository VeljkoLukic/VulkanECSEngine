#pragma once
#include "engine/core/Window.h"
#include "engine/ecs/Registry.h"
#include "engine/renderer/VulkanContext.h"
#include "engine/renderer/Swapchain.h"
#include "engine/renderer/Renderer.h"
#include "engine/systems/RenderSystem.h"
#include "engine/components/Camera.h"
#include <memory>
#include <functional>

namespace core {

class Engine {
public:
    Engine(int width, int height, const char* title);
    ~Engine();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    [[nodiscard]] ecs::Registry&       registry() { return m_registry; }
    [[nodiscard]] components::Camera&  camera()   { return m_camera; }
    [[nodiscard]] GLFWwindow*          window()   { return m_window.handle(); }
    [[nodiscard]] renderer::Renderer&  renderer() { return *m_renderer; }

    using UpdateFn = std::function<void(float dt)>;
    void setUpdateCallback(UpdateFn fn) { m_updateFn = std::move(fn); }

    void run();

private:
    void onResize(int width, int height);

    Window                             m_window;
    ecs::Registry                      m_registry;
    components::Camera                 m_camera;

    std::unique_ptr<renderer::VulkanContext> m_ctx;
    std::unique_ptr<renderer::Swapchain>     m_swapchain;
    std::unique_ptr<renderer::Renderer>      m_renderer;
    std::unique_ptr<systems::RenderSystem>   m_renderSystem;

    UpdateFn m_updateFn;
    bool     m_needsResize = false;
};

}
