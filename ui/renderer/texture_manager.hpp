// ui/renderer/texture_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>

namespace NexusForge::UI {

// Texture info
struct TextureInfo {
    uint64_t handle = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool hasMipmaps = false;
    std::string path;
    uint64_t lastModified = 0;
};

// Texture filter
enum class TextureFilter {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

// Texture wrap mode
enum class TextureWrap {
    ClampToEdge,
    Repeat,
    MirroredRepeat
};

// Texture manager
class TextureManager {
public:
    TextureManager();
    ~TextureManager();

    bool initialize();
    void shutdown();

    // Texture creation
    uint64_t createTexture(int width, int height, const void* data,
                           bool generateMipmaps = false);
    uint64_t createTexture(const std::string& path);

    // Texture loading
    uint64_t loadTexture(const std::string& path);
    uint64_t loadTextureFromMemory(const void* data, size_t size);
    bool reloadTexture(uint64_t handle);

    // Texture access
    const TextureInfo* getTextureInfo(uint64_t handle) const;
    int getTextureWidth(uint64_t handle) const;
    int getTextureHeight(uint64_t handle) const;

    // Texture modification
    void updateTexture(uint64_t handle, int x, int y, int width, int height,
                       const void* data);
    void generateMipmaps(uint64_t handle);

    // Texture deletion
    void destroyTexture(uint64_t handle);

    // Filtering
    void setFilter(uint64_t handle, TextureFilter filter);
    void setWrapMode(uint64_t handle, TextureWrap wrapS, TextureWrap wrapT);

    // Cache management
    void clearCache();
    void removeUnusedTextures();
    size_t getTextureCount() const { return textures_.size(); }
    size_t getMemoryUsage() const { return memoryUsage_; }

    // Default textures
    uint64_t getWhiteTexture() const { return whiteTexture_; }
    uint64_t getBlackTexture() const { return blackTexture_; }
    uint64_t getErrorTexture() const { return errorTexture_; }

private:
    std::unordered_map<uint64_t, TextureInfo> textures_;
    std::unordered_map<std::string, uint64_t> pathToHandle_;
    uint64_t nextHandle_ = 1;
    size_t memoryUsage_ = 0;

    uint64_t whiteTexture_ = 0;
    uint64_t blackTexture_ = 0;
    uint64_t errorTexture_ = 0;

    uint64_t createHandle();
    bool loadIntoTexture(uint64_t handle, const std::string& path);
    bool loadIntoTexture(uint64_t handle, const void* data, int width, int height, int channels);
};

// Image loader
class ImageLoader {
public:
    static unsigned char* load(const std::string& path, int* width, int* height,
                                int* channels, int desiredChannels = 0);
    static unsigned char* loadFromMemory(const void* data, size_t size,
                                          int* width, int* height, int* channels,
                                          int desiredChannels = 0);
    static void free(unsigned char* data);

    // Save
    static bool savePNG(const std::string& path, int width, int height,
                        int channels, const unsigned char* data);
    static bool saveJPEG(const std::string& path, int width, int height,
                         int channels, const unsigned char* data, int quality = 90);
};

} // namespace NexusForge::UI
