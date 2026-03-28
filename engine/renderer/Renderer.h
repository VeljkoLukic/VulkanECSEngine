#pragma once
#include "engine/renderer/VulkanContext.h"
#include "engine/renderer/Swapchain.h"
#include "engine/ecs/Types.h"
#include "engine/components/Mesh.h"
#include "engine/components/Camera.h"
#include "engine/components/Transform.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <array>

namespace renderer {

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct CameraUBO {
    glm::mat4 view;
    glm::mat4 proj;
};

struct GPUMesh {
    VkBuffer       vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer       indexBuffer  = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory  = VK_NULL_HANDLE;
    uint32_t       indexCount   = 0;
};

class Renderer {
public:
    Renderer(const VulkanContext& ctx, Swapchain& swapchain);
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    void uploadMesh(ecs::EntityID entity, const components::Mesh& mesh);
    void destroyMesh(ecs::EntityID entity);

    bool beginFrame();
    void updateCamera(const components::Camera& cam);
    void drawMesh(ecs::EntityID entity, const components::Transform& transform);
    bool endFrame();

    void onSwapchainRecreated();

private:
    void createDescriptorSetLayout();
    void createPipeline();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    [[nodiscard]] VkShaderModule loadShader(const std::string& path) const;

    const VulkanContext& m_ctx;
    Swapchain&           m_swapchain;

    VkDescriptorSetLayout m_descriptorLayout = VK_NULL_HANDLE;
    VkPipelineLayout      m_pipelineLayout   = VK_NULL_HANDLE;
    VkPipeline            m_pipeline         = VK_NULL_HANDLE;

    std::array<VkBuffer,        MAX_FRAMES_IN_FLIGHT> m_uniformBuffers {};
    std::array<VkDeviceMemory,  MAX_FRAMES_IN_FLIGHT> m_uniformMemory  {};
    std::array<void*,           MAX_FRAMES_IN_FLIGHT> m_uniformMapped  {};
    VkDescriptorPool                                   m_descriptorPool = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptorSets {};

    VkCommandPool                                      m_commandPool    = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>  m_commandBuffers {};

    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_imageAvailable {};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_renderFinished {};
    std::array<VkFence,     MAX_FRAMES_IN_FLIGHT> m_inFlight       {};

    int      m_currentFrame = 0;
    uint32_t m_imageIndex   = 0;
    bool     m_frameStarted = false;

    std::unordered_map<ecs::EntityID, GPUMesh> m_gpuMeshes;
};

} // namespace renderer
