#include "engine/renderer/Swapchain.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <array>
#include <string>

namespace renderer {

Swapchain::Swapchain(const VulkanContext& ctx, GLFWwindow* window) : m_ctx(ctx) {
    create(window);
}

Swapchain::~Swapchain() { destroy(); }

void Swapchain::recreate(GLFWwindow* window) {
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    while (w == 0 || h == 0) {
        glfwGetFramebufferSize(window, &w, &h);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(m_ctx.device());
    destroy();
    create(window);
}

uint32_t Swapchain::imageCount() const {
    return static_cast<uint32_t>(m_images.size());
}

void Swapchain::create(GLFWwindow* window) {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_ctx.physicalDevice(), m_ctx.surface(), &caps);

    uint32_t formatCount = 0, modeCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx.physicalDevice(), m_ctx.surface(), &formatCount, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx.physicalDevice(), m_ctx.surface(), &modeCount, nullptr);

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    std::vector<VkPresentModeKHR>   modes(modeCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx.physicalDevice(), m_ctx.surface(), &formatCount, formats.data());
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx.physicalDevice(), m_ctx.surface(), &modeCount, modes.data());

    auto surfaceFormat = chooseSurfaceFormat(formats);
    auto presentMode   = choosePresentMode(modes);
    m_extent           = chooseExtent(caps, window);
    m_imageFormat      = surfaceFormat.format;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0) imageCount = std::min(imageCount, caps.maxImageCount);

    auto families = m_ctx.queueFamilies();
    std::array<uint32_t, 2> familyIndices = { *families.graphics, *families.present };
    bool sharedQueue = *families.graphics == *families.present;

    VkSwapchainCreateInfoKHR ci {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = m_ctx.surface(),
        .minImageCount         = imageCount,
        .imageFormat           = surfaceFormat.format,
        .imageColorSpace       = surfaceFormat.colorSpace,
        .imageExtent           = m_extent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = sharedQueue ? VK_SHARING_MODE_EXCLUSIVE
                                             : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = sharedQueue ? 0u : 2u,
        .pQueueFamilyIndices   = sharedQueue ? nullptr : familyIndices.data(),
        .preTransform          = caps.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = presentMode,
        .clipped               = VK_TRUE,
    };

    VK_CHECK(vkCreateSwapchainKHR(m_ctx.device(), &ci, nullptr, &m_swapchain));

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(m_ctx.device(), m_swapchain, &count, nullptr);
    m_images.resize(count);
    vkGetSwapchainImagesKHR(m_ctx.device(), m_swapchain, &count, m_images.data());

    createImageViews();
    createDepthResources();
    createRenderPass();
    createFramebuffers();
}

void Swapchain::destroy() {
    auto dev = m_ctx.device();
    for (auto fb : m_framebuffers) vkDestroyFramebuffer(dev, fb, nullptr);
    m_framebuffers.clear();

    vkDestroyImageView(dev, m_depthView, nullptr);
    vkDestroyImage(dev, m_depthImage, nullptr);
    vkFreeMemory(dev, m_depthMemory, nullptr);
    m_depthView = VK_NULL_HANDLE; m_depthImage = VK_NULL_HANDLE; m_depthMemory = VK_NULL_HANDLE;

    for (auto iv : m_imageViews) vkDestroyImageView(dev, iv, nullptr);
    m_imageViews.clear();

    vkDestroyRenderPass(dev, m_renderPass, nullptr);
    m_renderPass = VK_NULL_HANDLE;

    vkDestroySwapchainKHR(dev, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
}

void Swapchain::createImageViews() {
    m_imageViews.resize(m_images.size());
    for (std::size_t i = 0; i < m_images.size(); ++i) {
        VkImageViewCreateInfo ci {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = m_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = m_imageFormat,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1, .layerCount = 1,
            },
        };
        VK_CHECK(vkCreateImageView(m_ctx.device(), &ci, nullptr, &m_imageViews[i]));
    }
}

VkFormat Swapchain::findDepthFormat() const {
    for (auto fmt : { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                      VK_FORMAT_D24_UNORM_S8_UINT }) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_ctx.physicalDevice(), fmt, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return fmt;
    }
    throw std::runtime_error("Failed to find suitable depth format");
}

void Swapchain::createDepthResources() {
    VkFormat depthFmt = findDepthFormat();

    VkImageCreateInfo imgInfo {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = depthFmt,
        .extent        = { m_extent.width, m_extent.height, 1 },
        .mipLevels     = 1, .arrayLayers = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VK_CHECK(vkCreateImage(m_ctx.device(), &imgInfo, nullptr, &m_depthImage));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(m_ctx.device(), m_depthImage, &memReqs);
    VkMemoryAllocateInfo alloc {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memReqs.size,
        .memoryTypeIndex = m_ctx.findMemoryType(memReqs.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    VK_CHECK(vkAllocateMemory(m_ctx.device(), &alloc, nullptr, &m_depthMemory));
    VK_CHECK(vkBindImageMemory(m_ctx.device(), m_depthImage, m_depthMemory, 0));

    VkImageViewCreateInfo viewInfo {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = m_depthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = depthFmt,
        .subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 },
    };
    VK_CHECK(vkCreateImageView(m_ctx.device(), &viewInfo, nullptr, &m_depthView));
}

void Swapchain::createRenderPass() {
    VkAttachmentDescription colorAttach {
        .format         = m_imageFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentDescription depthAttach {
        .format         = findDepthFormat(),
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference colorRef { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthRef { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &colorRef,
        .pDepthStencilAttachment = &depthRef,
    };

    VkSubpassDependency dep {
        .srcSubpass    = VK_SUBPASS_EXTERNAL, .dstSubpass = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                       | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                       | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                       | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    std::array<VkAttachmentDescription, 2> attachments = { colorAttach, depthAttach };
    VkRenderPassCreateInfo ci {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments    = attachments.data(),
        .subpassCount    = 1, .pSubpasses    = &subpass,
        .dependencyCount = 1, .pDependencies = &dep,
    };
    VK_CHECK(vkCreateRenderPass(m_ctx.device(), &ci, nullptr, &m_renderPass));
}

void Swapchain::createFramebuffers() {
    m_framebuffers.resize(m_imageViews.size());
    for (std::size_t i = 0; i < m_imageViews.size(); ++i) {
        std::array<VkImageView, 2> attachments = { m_imageViews[i], m_depthView };
        VkFramebufferCreateInfo ci {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = m_renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments    = attachments.data(),
            .width           = m_extent.width,
            .height          = m_extent.height,
            .layers          = 1,
        };
        VK_CHECK(vkCreateFramebuffer(m_ctx.device(), &ci, nullptr, &m_framebuffers[i]));
    }
}

VkSurfaceFormatKHR Swapchain::chooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& formats) const {
    for (auto& f : formats)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return f;
    return formats[0];
}

VkPresentModeKHR Swapchain::choosePresentMode(
    const std::vector<VkPresentModeKHR>& modes) const {
    for (auto m : modes)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& caps,
                                    GLFWwindow* window) const {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    return {
        std::clamp(static_cast<uint32_t>(w), caps.minImageExtent.width,  caps.maxImageExtent.width),
        std::clamp(static_cast<uint32_t>(h), caps.minImageExtent.height, caps.maxImageExtent.height),
    };
}

} // namespace renderer
