// ui/renderer/texture_manager.cpp
#include "texture_manager.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>

namespace NexusForge::UI {

TextureManager::TextureManager() = default;

TextureManager::~TextureManager() {
    shutdown();
}

bool TextureManager::initialize() {
    return true;
}

void TextureManager::shutdown() {
    unloadAllTextures();
}

TextureHandle TextureManager::createTexture(int width, int height, const void* data,
                                             bool generateMipmaps) {
    (void)data;
    (void)generateMipmaps;
    if (width <= 0 || height <= 0) return 0;

    auto info = std::make_unique<TextureInfo>();
    info->handle = nextHandle_;
    info->width = width;
    info->height = height;
    info->loaded = true;
    info->hasAlpha = true;

    textures_[nextHandle_] = std::move(info);
    memoryUsage_ += static_cast<size_t>(width) * static_cast<size_t>(height) * 4;

    return nextHandle_++;
}

TextureHandle TextureManager::createTextureFromColor(int width, int height,
                                                      const Color& color) {
    if (width <= 0 || height <= 0) return 0;

    auto toByte = [](float v) -> uint8_t {
        return static_cast<uint8_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };
    size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    std::vector<uint8_t> data(pixelCount * 4);
    uint8_t r = toByte(color.r);
    uint8_t g = toByte(color.g);
    uint8_t b = toByte(color.b);
    uint8_t a = toByte(color.a);

    for (size_t i = 0; i < pixelCount * 4; i += 4) {
        data[i] = r;
        data[i + 1] = g;
        data[i + 2] = b;
        data[i + 3] = a;
    }

    return createTexture(width, height, data.data());
}

TextureHandle TextureManager::loadTexture(const std::string& path) {
    // Check if already loaded
    auto it = pathToHandle_.find(path);
    if (it != pathToHandle_.end()) {
        return it->second;
    }
    
    auto info = std::make_unique<TextureInfo>();
    info->handle = nextHandle_;
    info->path = path;
    info->loaded = false;
    info->width = 1;
    info->height = 1;
    
    TextureHandle handle = nextHandle_++;
    
    if (loadTextureData(info.get())) {
        info->loaded = true;
        memoryUsage_ += info->width * info->height * 4;
    }
    
    pathToHandle_[path] = handle;
    textures_[handle] = std::move(info);
    
    return handle;
}

TextureHandle TextureManager::loadTextureAsync(const std::string& path) {
    // Would load asynchronously
    return loadTexture(path);
}

void TextureManager::destroyTexture(TextureHandle handle) {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        if (it->second) {
            size_t bytes = static_cast<size_t>(std::max(0, it->second->width)) *
                           static_cast<size_t>(std::max(0, it->second->height)) * 4;
            size_t current = memoryUsage_.load();
            memoryUsage_ = (current >= bytes) ? (current - bytes) : 0;

            // Remove from path cache
            if (!it->second->path.empty()) {
                pathToHandle_.erase(it->second->path);
            }
        }
        textures_.erase(it);
    }
}

void TextureManager::unloadAllTextures() {
    textures_.clear();
    pathToHandle_.clear();
    memoryUsage_ = 0;
    nextHandle_ = 1;
}

TextureInfo* TextureManager::getTextureInfo(TextureHandle handle) {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        return it->second.get();
    }
    return nullptr;
}

Vec2 TextureManager::getTextureSize(TextureHandle handle) {
    auto info = getTextureInfo(handle);
    if (info) {
        return Vec2(static_cast<float>(info->width), 
                    static_cast<float>(info->height));
    }
    return Vec2(0, 0);
}

bool TextureManager::isTextureLoaded(TextureHandle handle) const {
    auto it = textures_.find(handle);
    if (it != textures_.end() && it->second) {
        return it->second->loaded;
    }
    return false;
}

bool TextureManager::loadTextureData(TextureInfo* info) {
    // Would load image file (PNG, JPG, etc.)
    // For now, create a placeholder texture
    info->width = 64;
    info->height = 64;
    return true;
}

void TextureManager::calculateMemoryUsage() {
    memoryUsage_ = 0;
    for (const auto& [handle, info] : textures_) {
        if (info) {
            memoryUsage_ += info->width * info->height * 4;
        }
    }
}

} // namespace NexusForge::UI
