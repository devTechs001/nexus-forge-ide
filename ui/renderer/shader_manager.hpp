// ui/renderer/shader_manager.hpp
#pragma once

#include "render_engine.hpp"
#include <string>
#include <unordered_map>
#include <memory>

namespace NexusForge::UI {

// Shader program info
struct ShaderProgram {
    ShaderHandle handle;
    std::string name;
    std::string vertexSource;
    std::string fragmentSource;
    bool compiled;
    
    // Uniform locations
    int projectionMatrixLoc = -1;
    int modelMatrixLoc = -1;
    int colorLoc = -1;
    int textureLoc = -1;
};

// Shader Manager
class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();

    bool initialize();
    void shutdown();

    // Shader creation
    ShaderHandle createShader(const std::string& name,
                               const std::string& vertexSource,
                               const std::string& fragmentSource);
    
    // Shader loading
    ShaderHandle loadShader(const std::string& path);
    
    // Shader management
    void destroyShader(ShaderHandle handle);
    
    // Shader usage
    bool useShader(ShaderHandle handle);
    ShaderHandle getCurrentShader() const { return currentShader_; }
    
    // Uniform setting
    void setUniform1i(const std::string& name, int value);
    void setUniform1f(const std::string& name, float value);
    void setUniform2f(const std::string& name, float x, float y);
    void setUniform3f(const std::string& name, float x, float y, float z);
    void setUniform4f(const std::string& name, float x, float y, float z, float w);
    void setUniformMatrix4(const std::string& name, const Mat4& matrix);

    // Built-in shaders
    ShaderHandle getSolidColorShader() { return solidColorShader_; }
    ShaderHandle getTextureShader() { return textureShader_; }
    ShaderHandle getGradientShader() { return gradientShader_; }
    ShaderHandle getTextShader() { return textShader_; }

private:
    std::unordered_map<ShaderHandle, std::unique_ptr<ShaderProgram>> shaders_;
    ShaderHandle nextHandle_ = 1;
    ShaderHandle currentShader_ = 0;
    
    ShaderHandle solidColorShader_ = 0;
    ShaderHandle textureShader_ = 0;
    ShaderHandle gradientShader_ = 0;
    ShaderHandle textShader_ = 0;
    
    void createBuiltInShaders();
};

} // namespace NexusForge::UI
