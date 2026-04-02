#pragma once
#include <vulkan/vulkan.h>
struct GLFWwindow;
#include <optional>
#include <stdexcept>
#include <string>

#define VK_CHECK(expr)                                                          \
    do {                                                                        \
        VkResult _r = (expr);                                                   \
        if (_r != VK_SUCCESS)                                                   \
            throw std::runtime_error(                                           \
                std::string("Vulkan error ") + std::to_string(_r) +            \
                " in " __FILE__ ":" + std::to_string(__LINE__));               \
    } while (0)

namespace renderer {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    [[nodiscard]] bool complete() const {
        return graphics.has_value() && present.has_value();
    }
};

class VulkanContext {
public:
    explicit VulkanContext(GLFWwindow* window);
    ~VulkanContext();

    VulkanContext(const VulkanContext&)            = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    [[nodiscard]] VkInstance       instance()       const { return m_instance; }
    [[nodiscard]] VkSurfaceKHR     surface()        const { return m_surface; }
    [[nodiscard]] VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    [[nodiscard]] VkDevice         device()         const { return m_device; }
    [[nodiscard]] VkQueue          graphicsQueue()  const { return m_graphicsQueue; }
    [[nodiscard]] VkQueue          presentQueue()   const { return m_presentQueue; }
    [[nodiscard]] VkCommandPool    commandPool()    const { return m_commandPool; }

    [[nodiscard]] const QueueFamilyIndices& queueFamilies() const { return m_families; }

    void waitIdle() const { vkDeviceWaitIdle(m_device); }

    [[nodiscard]] uint32_t findMemoryType(uint32_t filter,
                                          VkMemoryPropertyFlags props) const;

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags props,
                      VkBuffer& buffer, VkDeviceMemory& memory) const;

    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const;

    [[nodiscard]] VkCommandBuffer beginOneTimeCmd() const;
    void endOneTimeCmd(VkCommandBuffer cmd) const;

private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();

    [[nodiscard]] bool                isDeviceSuitable(VkPhysicalDevice dev) const;
    [[nodiscard]] QueueFamilyIndices  findQueueFamilies(VkPhysicalDevice dev) const;
    [[nodiscard]] bool                checkValidationSupport() const;

    VkInstance               m_instance        = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger  = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface         = VK_NULL_HANDLE;
    VkPhysicalDevice         m_physicalDevice  = VK_NULL_HANDLE;
    VkDevice                 m_device          = VK_NULL_HANDLE;
    VkQueue                  m_graphicsQueue   = VK_NULL_HANDLE;
    VkQueue                  m_presentQueue    = VK_NULL_HANDLE;
    VkCommandPool            m_commandPool     = VK_NULL_HANDLE;
    QueueFamilyIndices       m_families;
};

}
