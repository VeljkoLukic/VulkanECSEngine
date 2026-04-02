#include "engine/renderer/VulkanContext.h"
#include <stdexcept>
#include <vector>
#include <set>
#include <cstring>
#include <iostream>
#include <string>
#include <GLFW/glfw3.h>

namespace renderer {

static const std::vector<const char*> k_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
static const std::vector<const char*> k_deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef VULKAN_VALIDATION
static constexpr bool k_enableValidation = true;
#else
static constexpr bool k_enableValidation = false;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*                                       /*userdata*/)
{
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cerr << "[Vulkan] " << data->pMessage << '\n';
    return VK_FALSE;
}

VulkanContext::VulkanContext(GLFWwindow* window) {
    createInstance();
    if (k_enableValidation) setupDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
}

VulkanContext::~VulkanContext() {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyDevice(m_device, nullptr);

    if (k_enableValidation && m_debugMessenger != VK_NULL_HANDLE) {
        auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (fn) fn(m_instance, m_debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void VulkanContext::createInstance() {
    if (k_enableValidation && !checkValidationSupport())
        throw std::runtime_error("Requested Vulkan validation layers unavailable");

    VkApplicationInfo appInfo {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "VulkanECSEngine",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName        = "NoEngine",
        .engineVersion      = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion         = VK_API_VERSION_1_2,
    };

    uint32_t     glfwExtCount = 0;
    const char** glfwExts     = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);
    if (k_enableValidation) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    VkInstanceCreateInfo ci {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = k_enableValidation
                                     ? static_cast<uint32_t>(k_validationLayers.size()) : 0,
        .ppEnabledLayerNames     = k_enableValidation ? k_validationLayers.data() : nullptr,
        .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VK_CHECK(vkCreateInstance(&ci, nullptr, &m_instance));
}

void VulkanContext::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT ci {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
    };

    auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
    if (!fn) throw std::runtime_error("Cannot load vkCreateDebugUtilsMessengerEXT");
    VK_CHECK(fn(m_instance, &ci, nullptr, &m_debugMessenger));
}

void VulkanContext::createSurface(GLFWwindow* window) {
    VK_CHECK(glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface));
}

void VulkanContext::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    if (count == 0) throw std::runtime_error("No Vulkan-capable GPU found");

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

    for (auto dev : devices) {
        if (isDeviceSuitable(dev)) {
            m_physicalDevice = dev;
            m_families       = findQueueFamilies(dev);
            return;
        }
    }
    throw std::runtime_error("No suitable GPU found");
}

void VulkanContext::createLogicalDevice() {
    std::set<uint32_t> uniqueQueueFamilies = { *m_families.graphics, *m_families.present };

    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    for (uint32_t family : uniqueQueueFamilies) {
        queueInfos.push_back({
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family,
            .queueCount       = 1,
            .pQueuePriorities = &priority,
        });
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo ci {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount    = static_cast<uint32_t>(queueInfos.size()),
        .pQueueCreateInfos       = queueInfos.data(),
        .enabledLayerCount       = k_enableValidation
                                     ? static_cast<uint32_t>(k_validationLayers.size()) : 0,
        .ppEnabledLayerNames     = k_enableValidation ? k_validationLayers.data() : nullptr,
        .enabledExtensionCount   = static_cast<uint32_t>(k_deviceExtensions.size()),
        .ppEnabledExtensionNames = k_deviceExtensions.data(),
        .pEnabledFeatures        = &features,
    };

    VK_CHECK(vkCreateDevice(m_physicalDevice, &ci, nullptr, &m_device));
    vkGetDeviceQueue(m_device, *m_families.graphics, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, *m_families.present,  0, &m_presentQueue);
}

void VulkanContext::createCommandPool() {
    VkCommandPoolCreateInfo ci {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = *m_families.graphics,
    };
    VK_CHECK(vkCreateCommandPool(m_device, &ci, nullptr, &m_commandPool));
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice dev) const {
    if (!findQueueFamilies(dev).complete()) return false;

    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> available(extCount);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, available.data());

    for (const char* required : k_deviceExtensions) {
        bool found = false;
        for (auto& ext : available)
            if (strcmp(required, ext.extensionName) == 0) { found = true; break; }
        if (!found) return false;
    }

    uint32_t formatCount = 0, modeCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &formatCount, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface, &modeCount, nullptr);
    return formatCount > 0 && modeCount > 0;
}

QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice dev) const {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());

    QueueFamilyIndices idx;
    for (uint32_t i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) idx.graphics = i;

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &presentSupport);
        if (presentSupport) idx.present = i;

        if (idx.complete()) break;
    }
    return idx;
}

bool VulkanContext::checkValidationSupport() const {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());

    for (const char* name : k_validationLayers) {
        bool found = false;
        for (auto& layer : layers)
            if (strcmp(name, layer.layerName) == 0) { found = true; break; }
        if (!found) return false;
    }
    return true;
}

uint32_t VulkanContext::findMemoryType(uint32_t filter,
                                       VkMemoryPropertyFlags props) const {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((filter & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    throw std::runtime_error("Failed to find suitable memory type");
}

void VulkanContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags props,
                                 VkBuffer& buffer, VkDeviceMemory& memory) const {
    VkBufferCreateInfo ci {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_CHECK(vkCreateBuffer(m_device, &ci, nullptr, &buffer));

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device, buffer, &memReqs);

    VkMemoryAllocateInfo alloc {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memReqs.size,
        .memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, props),
    };
    VK_CHECK(vkAllocateMemory(m_device, &alloc, nullptr, &memory));
    VK_CHECK(vkBindBufferMemory(m_device, buffer, memory, 0));
}

VkCommandBuffer VulkanContext::beginOneTimeCmd() const {
    VkCommandBufferAllocateInfo alloc {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    VK_CHECK(vkAllocateCommandBuffers(m_device, &alloc, &cmd));

    VkCommandBufferBeginInfo begin {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin));
    return cmd;
}

void VulkanContext::endOneTimeCmd(VkCommandBuffer cmd) const {
    VK_CHECK(vkEndCommandBuffer(cmd));
    VkSubmitInfo submit { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                          .commandBufferCount = 1, .pCommandBuffers = &cmd };
    VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submit, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(m_graphicsQueue));
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

void VulkanContext::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const {
    VkCommandBuffer cmd = beginOneTimeCmd();
    VkBufferCopy    region { .size = size };
    vkCmdCopyBuffer(cmd, src, dst, 1, &region);
    endOneTimeCmd(cmd);
}

}
