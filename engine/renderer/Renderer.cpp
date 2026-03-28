#include "engine/renderer/Renderer.h"
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace renderer {

Renderer::Renderer(const VulkanContext& ctx, Swapchain& swapchain)
    : m_ctx(ctx), m_swapchain(swapchain)
{
    createDescriptorSetLayout();
    createPipeline();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(m_ctx.device());

    for (auto& [id, gpu] : m_gpuMeshes) {
        vkDestroyBuffer(m_ctx.device(), gpu.vertexBuffer, nullptr);
        vkFreeMemory(m_ctx.device(), gpu.vertexMemory, nullptr);
        vkDestroyBuffer(m_ctx.device(), gpu.indexBuffer, nullptr);
        vkFreeMemory(m_ctx.device(), gpu.indexMemory, nullptr);
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(m_ctx.device(), m_imageAvailable[i], nullptr);
        vkDestroySemaphore(m_ctx.device(), m_renderFinished[i], nullptr);
        vkDestroyFence(m_ctx.device(), m_inFlight[i], nullptr);
        vkDestroyBuffer(m_ctx.device(), m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_ctx.device(), m_uniformMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(m_ctx.device(), m_descriptorPool, nullptr);
    vkDestroyCommandPool(m_ctx.device(), m_commandPool, nullptr);
    vkDestroyPipeline(m_ctx.device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_ctx.device(), m_descriptorLayout, nullptr);
}

void Renderer::uploadMesh(ecs::EntityID entity, const components::Mesh& mesh) {
    if (m_gpuMeshes.count(entity)) return;

    GPUMesh gpu;
    gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());

    auto uploadBuffer = [&](const void* data, VkDeviceSize size,
                            VkBufferUsageFlags usage,
                            VkBuffer& buf, VkDeviceMemory& mem) {
        VkBuffer       staging;
        VkDeviceMemory stagingMem;
        m_ctx.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           staging, stagingMem);

        void* mapped;
        vkMapMemory(m_ctx.device(), stagingMem, 0, size, 0, &mapped);
        std::memcpy(mapped, data, size);
        vkUnmapMemory(m_ctx.device(), stagingMem);

        m_ctx.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buf, mem);
        m_ctx.copyBuffer(staging, buf, size);

        vkDestroyBuffer(m_ctx.device(), staging, nullptr);
        vkFreeMemory(m_ctx.device(), stagingMem, nullptr);
    };

    VkDeviceSize vSize = mesh.vertices.size() * sizeof(components::Vertex);
    VkDeviceSize iSize = mesh.indices.size()  * sizeof(uint32_t);

    uploadBuffer(mesh.vertices.data(), vSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 gpu.vertexBuffer, gpu.vertexMemory);
    uploadBuffer(mesh.indices.data(),  iSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 gpu.indexBuffer, gpu.indexMemory);

    m_gpuMeshes[entity] = gpu;
}

void Renderer::destroyMesh(ecs::EntityID entity) {
    auto it = m_gpuMeshes.find(entity);
    if (it == m_gpuMeshes.end()) return;
    vkDeviceWaitIdle(m_ctx.device());
    auto& gpu = it->second;
    vkDestroyBuffer(m_ctx.device(), gpu.vertexBuffer, nullptr);
    vkFreeMemory(m_ctx.device(), gpu.vertexMemory, nullptr);
    vkDestroyBuffer(m_ctx.device(), gpu.indexBuffer, nullptr);
    vkFreeMemory(m_ctx.device(), gpu.indexMemory, nullptr);
    m_gpuMeshes.erase(it);
}

bool Renderer::beginFrame() {
    vkWaitForFences(m_ctx.device(), 1, &m_inFlight[m_currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(
        m_ctx.device(), m_swapchain.handle(), UINT64_MAX,
        m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) return false;
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swapchain image");

    vkResetFences(m_ctx.device(), 1, &m_inFlight[m_currentFrame]);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin));

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].color        = { { 0.05f, 0.05f, 0.05f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rpInfo {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass      = m_swapchain.renderPass(),
        .framebuffer     = m_swapchain.framebuffer(m_imageIndex),
        .renderArea      = { .offset = {0,0}, .extent = m_swapchain.extent() },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues    = clearValues.data(),
    };
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport vp { 0, 0,
                    static_cast<float>(m_swapchain.extent().width),
                    static_cast<float>(m_swapchain.extent().height),
                    0.0f, 1.0f };
    VkRect2D   sc { {0,0}, m_swapchain.extent() };
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                             0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);

    m_frameStarted = true;
    return true;
}

void Renderer::updateCamera(const components::Camera& cam) {
    float aspect = static_cast<float>(m_swapchain.extent().width) /
                   static_cast<float>(m_swapchain.extent().height);
    CameraUBO ubo { cam.view(), cam.projection(aspect) };
    std::memcpy(m_uniformMapped[m_currentFrame], &ubo, sizeof(ubo));
}

void Renderer::drawMesh(ecs::EntityID entity, const components::Transform& transform) {
    auto it = m_gpuMeshes.find(entity);
    if (it == m_gpuMeshes.end()) return;

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    const GPUMesh&  gpu = it->second;

    glm::mat4 model = transform.matrix();
    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(glm::mat4), glm::value_ptr(model));

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &gpu.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, gpu.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, gpu.indexCount, 1, 0, 0, 0);
}

bool Renderer::endFrame() {
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkCmdEndRenderPass(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &m_imageAvailable[m_currentFrame],
        .pWaitDstStageMask    = &waitStage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &m_renderFinished[m_currentFrame],
    };
    VK_CHECK(vkQueueSubmit(m_ctx.graphicsQueue(), 1, &submit, m_inFlight[m_currentFrame]));

    VkSwapchainKHR swapchainHandle = m_swapchain.handle();
    VkPresentInfoKHR present {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &m_renderFinished[m_currentFrame],
        .swapchainCount     = 1,
        .pSwapchains        = &swapchainHandle,
        .pImageIndices      = &m_imageIndex,
    };
    VkResult presentResult = vkQueuePresentKHR(m_ctx.presentQueue(), &present);

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    m_frameStarted = false;

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR) {
        return false;
    }
    if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }
    return true;
}

void Renderer::onSwapchainRecreated() {
    vkDestroyPipeline(m_ctx.device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_ctx.device(), m_pipelineLayout, nullptr);
    m_pipeline       = VK_NULL_HANDLE;
    m_pipelineLayout = VK_NULL_HANDLE;
    createPipeline();
}

void Renderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding {
        .binding         = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutCreateInfo ci {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings    = &uboBinding,
    };
    VK_CHECK(vkCreateDescriptorSetLayout(m_ctx.device(), &ci, nullptr, &m_descriptorLayout));
}

VkShaderModule Renderer::loadShader(const std::string& path) const {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Cannot open shader: " + path);

    std::size_t size = static_cast<std::size_t>(file.tellg());
    std::vector<char> code(size);
    file.seekg(0);
    file.read(code.data(), static_cast<std::streamsize>(size));

    VkShaderModuleCreateInfo ci {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode    = reinterpret_cast<const uint32_t*>(code.data()),
    };
    VkShaderModule mod;
    VK_CHECK(vkCreateShaderModule(m_ctx.device(), &ci, nullptr, &mod));
    return mod;
}

void Renderer::createPipeline() {
    auto vertModule = loadShader(std::string(SHADER_DIR) + "mesh.vert.spv");
    auto fragModule = loadShader(std::string(SHADER_DIR) + "mesh.frag.spv");

    VkPipelineShaderStageCreateInfo stages[] = {
        { .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage  = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vertModule, .pName = "main" },
        { .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fragModule, .pName = "main" },
    };

    auto bindDesc  = components::Vertex::bindingDescription();
    auto attrDescs = components::Vertex::attributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInput {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindDesc,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size()),
        .pVertexAttributeDescriptions    = attrDescs.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    std::array<VkDynamicState, 2> dynStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynState {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynStates.size()),
        .pDynamicStates    = dynStates.data(),
    };

    VkPipelineViewportStateCreateInfo viewportState {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1, .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer {
        .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode    = VK_CULL_MODE_BACK_BIT,
        .frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth   = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable  = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp   = VK_COMPARE_OP_LESS,
    };

    VkPipelineColorBlendAttachmentState blendAttach {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo blending {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments    = &blendAttach,
    };

    VkPushConstantRange pushRange {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset     = 0,
        .size       = sizeof(glm::mat4),
    };

    VkPipelineLayoutCreateInfo layoutCI {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 1,
        .pSetLayouts            = &m_descriptorLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pushRange,
    };
    VK_CHECK(vkCreatePipelineLayout(m_ctx.device(), &layoutCI, nullptr, &m_pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineCI {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = stages,
        .pVertexInputState   = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = &depthStencil,
        .pColorBlendState    = &blending,
        .pDynamicState       = &dynState,
        .layout              = m_pipelineLayout,
        .renderPass          = m_swapchain.renderPass(),
        .subpass             = 0,
    };
    VK_CHECK(vkCreateGraphicsPipelines(m_ctx.device(), VK_NULL_HANDLE,
                                       1, &pipelineCI, nullptr, &m_pipeline));

    vkDestroyShaderModule(m_ctx.device(), vertModule, nullptr);
    vkDestroyShaderModule(m_ctx.device(), fragModule, nullptr);
}

void Renderer::createUniformBuffers() {
    constexpr VkDeviceSize size = sizeof(CameraUBO);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_ctx.createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           m_uniformBuffers[i], m_uniformMemory[i]);
        vkMapMemory(m_ctx.device(), m_uniformMemory[i], 0, size, 0, &m_uniformMapped[i]);
    }
}

void Renderer::createDescriptorPool() {
    VkDescriptorPoolSize poolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT };
    VkDescriptorPoolCreateInfo ci {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .poolSizeCount = 1, .pPoolSizes = &poolSize,
    };
    VK_CHECK(vkCreateDescriptorPool(m_ctx.device(), &ci, nullptr, &m_descriptorPool));
}

void Renderer::createDescriptorSets() {
    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
    layouts.fill(m_descriptorLayout);

    VkDescriptorSetAllocateInfo alloc {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts        = layouts.data(),
    };
    VK_CHECK(vkAllocateDescriptorSets(m_ctx.device(), &alloc, m_descriptorSets.data()));

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufInfo { m_uniformBuffers[i], 0, sizeof(CameraUBO) };
        VkWriteDescriptorSet   write {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = m_descriptorSets[i],
            .dstBinding      = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo     = &bufInfo,
        };
        vkUpdateDescriptorSets(m_ctx.device(), 1, &write, 0, nullptr);
    }
}

void Renderer::createCommandBuffers() {
    VkCommandPoolCreateInfo poolCI {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = *m_ctx.queueFamilies().graphics,
    };
    VK_CHECK(vkCreateCommandPool(m_ctx.device(), &poolCI, nullptr, &m_commandPool));

    VkCommandBufferAllocateInfo alloc {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size()),
    };
    VK_CHECK(vkAllocateCommandBuffers(m_ctx.device(), &alloc, m_commandBuffers.data()));
}

void Renderer::createSyncObjects() {
    VkSemaphoreCreateInfo semCI { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo     fenCI { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                  .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VK_CHECK(vkCreateSemaphore(m_ctx.device(), &semCI, nullptr, &m_imageAvailable[i]));
        VK_CHECK(vkCreateSemaphore(m_ctx.device(), &semCI, nullptr, &m_renderFinished[i]));
        VK_CHECK(vkCreateFence(m_ctx.device(), &fenCI, nullptr, &m_inFlight[i]));
    }
}

} // namespace renderer
