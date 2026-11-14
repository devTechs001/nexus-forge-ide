// ui/renderer/shader_manager.cpp
#include "shader_manager.hpp"

namespace NexusForge::UI {

ShaderManager::ShaderManager() = default;

ShaderManager::~ShaderManager() {
    shutdown();
}

bool ShaderManager::initialize() {
    createBuiltInShaders();
    return true;
}

void ShaderManager::shutdown() {
    shaders_.clear();
}

ShaderHandle ShaderManager::createShader(const std::string& name,
                                          const std::string& vertexSource,
                                          const std::string& fragmentSource) {
    ShaderHandle handle = nextHandle_++;
    auto program = std::make_unique<ShaderProgram>();
    program->handle = handle;
    program->name = name;
    program->vertexSource = vertexSource;
    program->fragmentSource = fragmentSource;
    program->compiled = true;

    shaders_[handle] = std::move(program);

    return handle;
}

ShaderHandle ShaderManager::loadShader(const std::string& path) {
    return createShader(path,
        "#version 330 core\nlayout(location=0) in vec2 pos;\nvoid main(){gl_Position=vec4(pos,0,1);}",
        "#version 330 core\nout vec4 color;\nvoid main(){color=vec4(1,0,0,1);}");
}

void ShaderManager::destroyShader(ShaderHandle handle) {
    shaders_.erase(handle);
}

bool ShaderManager::useShader(ShaderHandle handle) {
    auto it = shaders_.find(handle);
    if (it != shaders_.end() && it->second->compiled) {
        currentShader_ = handle;
        return true;
    }
    return false;
}

void ShaderManager::setUniform1i(const std::string& name, int value) {
    // Would set uniform on current shader
}

void ShaderManager::setUniform1f(const std::string& name, float value) {
    // Would set uniform on current shader
}

void ShaderManager::setUniform2f(const std::string& name, float x, float y) {
    // Would set uniform on current shader
}

void ShaderManager::setUniform3f(const std::string& name, float x, float y, float z) {
    // Would set uniform on current shader
}

void ShaderManager::setUniform4f(const std::string& name, float x, float y, float z, float w) {
    // Would set uniform on current shader
}

void ShaderManager::setUniformMatrix4(const std::string& name, const Mat4& matrix) {
    // Would set uniform on current shader
}

void ShaderManager::createBuiltInShaders() {
    // Solid color shader
    solidColorShader_ = createShader("solid_color",
        R"(#version 330 core
        layout(location = 0) in vec2 position;
        uniform mat4 projection;
        uniform mat4 model;
        void main() {
            gl_Position = projection * model * vec4(position, 0.0, 1.0);
        })",
        R"(#version 330 core
        out vec4 fragColor;
        uniform vec4 color;
        void main() {
            fragColor = color;
        })"
    );
    
    // Texture shader
    textureShader_ = createShader("texture",
        R"(#version 330 core
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec2 texCoord;
        uniform mat4 projection;
        uniform mat4 model;
        out vec2 uv;
        void main() {
            gl_Position = projection * model * vec4(position, 0.0, 1.0);
            uv = texCoord;
        })",
        R"(#version 330 core
        in vec2 uv;
        out vec4 fragColor;
        uniform sampler2D texture0;
        uniform vec4 color;
        void main() {
            fragColor = texture(texture0, uv) * color;
        })"
    );
    
    // Gradient shader
    gradientShader_ = createShader("gradient",
        R"(#version 330 core
        layout(location = 0) in vec2 position;
        uniform mat4 projection;
        uniform mat4 model;
        out vec2 uv;
        void main() {
            gl_Position = projection * model * vec4(position, 0.0, 1.0);
            uv = position;
        })",
        R"(#version 330 core
        in vec2 uv;
        out vec4 fragColor;
        uniform vec4 color1;
        uniform vec4 color2;
        uniform int gradientType;
        void main() {
            float t = gradientType == 0 ? uv.y : uv.x;
            fragColor = mix(color1, color2, t);
        })"
    );
    
    // Text shader
    textShader_ = createShader("text",
        R"(#version 330 core
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec2 texCoord;
        uniform mat4 projection;
        uniform mat4 model;
        out vec2 uv;
        void main() {
            gl_Position = projection * model * vec4(position, 0.0, 1.0);
            uv = texCoord;
        })",
        R"(#version 330 core
        in vec2 uv;
        out vec4 fragColor;
        uniform sampler2D texture0;
        uniform vec4 color;
        void main() {
            float alpha = texture(texture0, uv).r;
            fragColor = vec4(color.rgb, alpha * color.a);
        })"
    );
}

} // namespace NexusForge::UI
