// ui/renderer/shader_manager.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>

namespace NexusForge::UI {

// Shader type
enum class ShaderType {
    Vertex,
    Fragment,
    Geometry,
    Compute
};

// Shader program
struct ShaderProgram {
    uint64_t handle = 0;
    std::string name;
    std::string vertexSource;
    std::string fragmentSource;
    bool compiled = false;
    bool linked = false;
    std::string compileLog;
    std::string linkLog;

    // Uniform locations
    std::unordered_map<std::string, int> uniformLocations;
};

// Shader manager
class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();

    bool initialize();
    void shutdown();

    // Shader creation
    uint64_t createShader(const std::string& name,
                          const std::string& vertexSource,
                          const std::string& fragmentSource);
    uint64_t createShaderFromFile(const std::string& name,
                                   const std::string& vertexPath,
                                   const std::string& fragmentPath);

    // Shader access
    ShaderProgram* getShader(uint64_t handle);
    ShaderProgram* getShader(const std::string& name);
    void useShader(uint64_t handle);

    // Uniforms
    void setUniformInt(uint64_t shader, const std::string& name, int value);
    void setUniformFloat(uint64_t shader, const std::string& name, float value);
    void setUniformVec2(uint64_t shader, const std::string& name, float x, float y);
    void setUniformVec3(uint64_t shader, const std::string& name, float x, float y, float z);
    void setUniformVec4(uint64_t shader, const std::string& name,
                        float x, float y, float z, float w);
    void setUniformMat4(uint64_t shader, const std::string& name, const float* matrix);

    // Shader deletion
    void destroyShader(uint64_t handle);

    // Built-in shaders
    uint64_t getSolidColorShader() const { return solidColorShader_; }
    uint64_t getTexturedShader() const { return texturedShader_; }
    uint64_t getGradientShader() const { return gradientShader_; }
    uint64_t getRoundedRectShader() const { return roundedRectShader_; }

private:
    std::unordered_map<uint64_t, ShaderProgram> shaders_;
    std::unordered_map<std::string, uint64_t> nameToHandle_;
    uint64_t nextHandle_ = 1;

    uint64_t solidColorShader_ = 0;
    uint64_t texturedShader_ = 0;
    uint64_t gradientShader_ = 0;
    uint64_t roundedRectShader_ = 0;

    uint64_t createHandle();
    bool compileShader(uint64_t shader, ShaderType type, const std::string& source);
    bool linkShader(uint64_t shader);
    int getUniformLocation(uint64_t shader, const std::string& name);

    void createBuiltInShaders();
};

// Shader source generators
namespace ShaderSources {
    std::string getVertexShader2D();
    std::string getFragmentShaderSolidColor();
    std::string getFragmentShaderTextured();
    std::string getFragmentShaderGradient();
    std::string getFragmentShaderRoundedRect();
}

} // namespace NexusForge::UI
