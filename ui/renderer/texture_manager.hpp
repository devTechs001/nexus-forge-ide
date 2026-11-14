// ui/renderer/texture_manager.hpp
#pragma once

#include "render_engine.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <atomic>

namespace NexusForge::UI {

// Texture information
struct TextureInfo {
    TextureHandle handle = 0;
    std::string path;
    int width = 0;
    int height = 0;
    bool loaded = false;
    bool hasAlpha = true;
};

// Texture Manager
class TextureManager {
public:
    TextureManager();
    ~TextureManager();

    bool initialize();
    void shutdown();

    // Texture creation
    TextureHandle createTexture(int width, int height, const void* data,
                                bool generateMipmaps = false);
    TextureHandle createTextureFromColor(int width, int height, const Color& color);

    // Texture loading
    TextureHandle loadTexture(const std::string& path);
    TextureHandle loadTextureAsync(const std::string& path);
    void destroyTexture(TextureHandle handle);
    void unloadAllTextures();

    // Texture info
    TextureInfo* getTextureInfo(TextureHandle handle);
    Vec2 getTextureSize(TextureHandle handle);
    bool isTextureLoaded(TextureHandle handle) const;

    // Statistics
    size_t getMemoryUsage() const { return memoryUsage_; }
    size_t getTextureCount() const { return textures_.size(); }

private:
    std::unordered_map<TextureHandle, std::unique_ptr<TextureInfo>> textures_;
    std::unordered_map<std::string, TextureHandle> pathToHandle_;
    TextureHandle nextHandle_ = 1;
    std::atomic<size_t> memoryUsage_{0};

    bool loadTextureData(TextureInfo* info);
    void calculateMemoryUsage();
};

} // namespace NexusForge::UI
