// ui/renderer/texture_manager.cpp
#include "texture_manager.hpp"
#include <cstring>

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
    auto info = std::make_unique<TextureInfo>();
    info->handle = nextHandle_;
    info->width = width;
    info->height = height;
    info->loaded = true;
    
    textures_[nextHandle_] = std::move(info);
    memoryUsage_ += width * height * 4; // RGBA
    
    return nextHandle_++;
}

TextureHandle TextureManager::createTextureFromColor(int width, int height, 
                                                      const Color& color) {
    std::vector<uint8_t> data(width * height * 4);
    uint8_t r = static_cast<uint8_t>(color.r * 255);
    uint8_t g = static_cast<uint8_t>(color.g * 255);
    uint8_t b = static_cast<uint8_t>(color.b * 255);
    uint8_t a = static_cast<uint8_t>(color.a * 255);
    
    for (int i = 0; i < width * height * 4; i += 4) {
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
            memoryUsage_ -= it->second->width * it->second->height * 4;
            
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
