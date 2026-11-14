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

    bool initialize() override { return initialize(nullptr); }
    bool initialize(void* windowHandle);
    void shutdown() override;

    void beginFrame() override;
    void endFrame() override;
    void present() override;

    void setViewport(int x, int y, int width, int height) override;
    void setProjection(const Mat4& projection);
    void clear(const Color& color) override;
    void draw(const DrawCommand& cmd) override { executeDrawCommand(cmd); }
    void executeDrawCommand(const DrawCommand& cmd) override;

    void drawRect(const Rect& rect, const Color& color);
    void drawRoundedRect(const Rect& rect, const Color& color, float radius);
    void drawTexture(TextureHandle texture, const Rect& rect, const Color& tint);
    void drawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness);
    void drawCircle(float cx, float cy, float radius, const Color& color, bool filled);
    void drawText(const std::string& text, float x, float y, const Color& color,
                  const std::string& fontFamily, float fontSize);
    void setClipRect(const Rect& rect);
    void clearClipRect();

    TextureHandle createTexture(int width, int height, const void* data);
    void destroyTexture(TextureHandle texture);
    void updateTexture(TextureHandle texture, int width, int height, const void* data);

    bool isInitialized() const;
    std::string getRendererInfo() const;

    // Vulkan-specific
    VkInstance getInstance() const { return instance_; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }
    VkDevice getDevice() const { return device_; }
    VkQueue getGraphicsQueue() const { return graphicsQueue_; }
    VkRenderPass getRenderPass() const { return renderPass_; }
    VkPipeline getPipeline() const { return pipeline_; }

private:
    bool initialized_ = false;
    void* windowHandle_ = nullptr;
    int viewportX_ = 0;
    int viewportY_ = 0;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    Mat4 projection_;
    Color clearColor_;

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
