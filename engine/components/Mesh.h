#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

namespace components {

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;

    [[nodiscard]] static VkVertexInputBindingDescription bindingDescription() {
        return {
            .binding   = 0,
            .stride    = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }

    [[nodiscard]] static std::array<VkVertexInputAttributeDescription, 2>
    attributeDescriptions() {
        return {{
            { .location = 0, .binding = 0,
              .format   = VK_FORMAT_R32G32B32_SFLOAT,
              .offset   = offsetof(Vertex, position) },
            { .location = 1, .binding = 0,
              .format   = VK_FORMAT_R32G32B32_SFLOAT,
              .offset   = offsetof(Vertex, color) },
        }};
    }
};

struct Mesh {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
};

} // namespace components
