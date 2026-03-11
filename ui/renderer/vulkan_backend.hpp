// ui/renderer/vulkan_backend.hpp
#pragma once

#include "render_engine.hpp"

#ifdef NEXUS_ENABLE_VULKAN

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace NexusForge::UI {

// Vulkan render backend
class VulkanBackend : public RenderBackend {
public:
    VulkanBackend();
    ~VulkanBackend() override;

    bool initialize() override;
    void shutdown() override;

    void setViewport(int x, int y, int width, int height) override;
    void clear(const Color& color) override;
    void present() override;

    void executeDrawCommand(const DrawCommand& cmd) override;

    // Vulkan-specific
    VkInstance getInstance() const { return instance_; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }
    VkDevice getDevice() const { return device_; }
    VkQueue getGraphicsQueue() const { return graphicsQueue_; }
    VkRenderPass getRenderPass() const { return renderPass_; }
    VkPipeline getPipeline() const { return pipeline_; }

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;

    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> framebuffers_;
    std::vector<VkImageView> imageViews_;
    std::vector<VkImage> images_;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;

    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;

    bool debug_ = false;

    bool createInstance();
    bool createSurface(void* window);
    bool selectPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain();
    bool createImageViews();
    bool createRenderPass();
    bool createPipeline();
    bool createFramebuffers();
    bool createCommandPool();
    bool createDescriptorPool();
    bool createSyncObjects();

    void cleanupSwapchain();
    void recreateSwapchain();

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
};

} // namespace NexusForge::UI

#endif // NEXUS_ENABLE_VULKAN
