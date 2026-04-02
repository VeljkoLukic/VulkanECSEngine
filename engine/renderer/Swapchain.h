#pragma once
#include "engine/renderer/VulkanContext.h"
#include <vulkan/vulkan.h>
struct GLFWwindow;
#include <vector>

namespace renderer {

class Swapchain {
public:
    Swapchain(const VulkanContext& ctx, GLFWwindow* window);
    ~Swapchain();

    Swapchain(const Swapchain&)            = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    void recreate(GLFWwindow* window);

    [[nodiscard]] VkSwapchainKHR handle()     const { return m_swapchain; }
    [[nodiscard]] VkRenderPass   renderPass() const { return m_renderPass; }
    [[nodiscard]] VkExtent2D     extent()     const { return m_extent; }
    [[nodiscard]] VkFormat       imageFormat() const { return m_imageFormat; }
    [[nodiscard]] uint32_t       imageCount() const;

    [[nodiscard]] VkFramebuffer framebuffer(uint32_t index) const {
        return m_framebuffers[index];
    }

private:
    void create(GLFWwindow* window);
    void destroy();

    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();

    [[nodiscard]] VkFormat findDepthFormat() const;

    [[nodiscard]] VkSurfaceFormatKHR chooseSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& formats) const;

    [[nodiscard]] VkPresentModeKHR choosePresentMode(
        const std::vector<VkPresentModeKHR>& modes) const;

    [[nodiscard]] VkExtent2D chooseExtent(
        const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) const;

    const VulkanContext& m_ctx;

    VkSwapchainKHR           m_swapchain  = VK_NULL_HANDLE;
    VkFormat                 m_imageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D               m_extent     = {};
    VkRenderPass             m_renderPass = VK_NULL_HANDLE;

    std::vector<VkImage>       m_images;
    std::vector<VkImageView>   m_imageViews;
    std::vector<VkFramebuffer> m_framebuffers;

    VkImage        m_depthImage  = VK_NULL_HANDLE;
    VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
    VkImageView    m_depthView   = VK_NULL_HANDLE;
};

}
