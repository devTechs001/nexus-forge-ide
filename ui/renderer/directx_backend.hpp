// ui/renderer/directx_backend.hpp
#pragma once

#include "render_engine.hpp"

#ifdef NEXUS_ENABLE_DX12

#include <d3d11.h>
#include <dxgi.h>
#include <vector>
#include <string>
#include <unordered_map>

namespace NexusForge::UI {

// DirectX render backend
class DirectXBackend : public RenderBackend {
public:
    DirectXBackend();
    ~DirectXBackend() override;

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

    ID3D11Device* getDevice() const { return device_; }

private:
    struct TextureData {
        ID3D11Texture2D* resource = nullptr;
        ID3D11ShaderResourceView* shaderResourceView = nullptr;
    };

    bool initialized_ = false;
    void* windowHandle_ = nullptr;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    Mat4 projection_;
    Color clearColor_;

    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* renderTargetView_ = nullptr;
    ID3D11VertexShader* vertexShader_ = nullptr;
    ID3D11PixelShader* pixelShader_ = nullptr;
    ID3D11PixelShader* pixelShaderColor_ = nullptr;
    ID3D11PixelShader* pixelShaderTexture_ = nullptr;
    ID3D11InputLayout* inputLayout_ = nullptr;
    ID3D11Buffer* constantBuffer_ = nullptr;
    ID3D11SamplerState* samplerState_ = nullptr;
    ID3D11BlendState* blendState_ = nullptr;
    ID3D11Buffer* vertexBuffer_ = nullptr;
    ID3D11Buffer* indexBuffer_ = nullptr;
    ID3D11ShaderResourceView* currentTexture_ = nullptr;
    UINT vertexStride_ = 0;
    UINT vertexOffset_ = 0;

    std::unordered_map<TextureHandle, TextureData> textures_;
    TextureHandle nextTextureHandle_ = 1;

    void createShaders();
    void createBuffers();
};

} // namespace NexusForge::UI

#endif // NEXUS_ENABLE_DX12
