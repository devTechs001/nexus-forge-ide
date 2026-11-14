// ui/renderer/directx_backend.cpp
// DirectX 11 Backend Implementation

#include "directx_backend.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef NEXUS_ENABLE_DX12

#define COBJMACROS
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

namespace NexusForge::UI {

struct DXVertex {
    float x, y, z;
    float u, v;
    float r, g, b, a;
};

struct ConstantBuffer {
    DirectX::XMMATRIX projection;
    DirectX::XMMATRIX transform;
};

DirectXBackend::DirectXBackend() 
    : initialized_(false), 
      device_(nullptr), 
      context_(nullptr),
      swapChain_(nullptr),
      renderTargetView_(nullptr),
      vertexShader_(nullptr),
      pixelShader_(nullptr),
      inputLayout_(nullptr),
      constantBuffer_(nullptr),
      samplerState_(nullptr),
      blendState_(nullptr),
      vertexBuffer_(nullptr),
      indexBuffer_(nullptr),
      currentTexture_(nullptr) {
}

DirectXBackend::~DirectXBackend() {
    shutdown();
}

bool DirectXBackend::initialize(void* windowHandle) {
    windowHandle_ = windowHandle;
    
    HRESULT hr;
    
    // Create D3D11 device
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &device_,
        &featureLevel,
        &context_
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Create swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = static_cast<HWND>(windowHandle_);
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    
    IDXGIDevice* dxgiDevice = nullptr;
    device_->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    
    IDXGIAdapter* adapter = nullptr;
    dxgiDevice->GetAdapter(&adapter);
    
    IDXGIFactory* factory = nullptr;
    adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));
    
    hr = factory->CreateSwapChain(device_, &swapChainDesc, &swapChain_);
    
    factory->Release();
    adapter->Release();
    dxgiDevice->Release();
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Create render target view
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
    backBuffer->Release();
    
    // Set render target
    context_->OMSetRenderTargets(1, &renderTargetView_, nullptr);
    
    // Create shaders
    createShaders();
    
    // Create vertex buffer
    createBuffers();
    
    // Create sampler state
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    device_->CreateSamplerState(&samplerDesc, &samplerState_);
    
    // Create blend state
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device_->CreateBlendState(&blendDesc, &blendState_);
    
    // Set blend state
    float blendFactor[] = { 0, 0, 0, 0 };
    context_->OMSetBlendState(blendState_, blendFactor, 0xFFFFFFFF);
    
    // Set primitive topology
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    initialized_ = true;
    return true;
}

void DirectXBackend::shutdown() {
    if (!initialized_) return;
    
    // Release all resources
    if (indexBuffer_) indexBuffer_->Release();
    if (vertexBuffer_) vertexBuffer_->Release();
    if (blendState_) blendState_->Release();
    if (samplerState_) samplerState_->Release();
    if (constantBuffer_) constantBuffer_->Release();
    if (inputLayout_) inputLayout_->Release();
    if (pixelShader_) pixelShader_->Release();
    if (vertexShader_) vertexShader_->Release();
    if (renderTargetView_) renderTargetView_->Release();
    if (swapChain_) swapChain_->Release();
    if (context_) context_->Release();
    if (device_) device_->Release();
    
    for (auto& [handle, texture] : textures_) {
        if (texture.resource) texture.resource->Release();
        if (texture.shaderResourceView) texture.shaderResourceView->Release();
    }
    textures_.clear();
    
    initialized_ = false;
}

void DirectXBackend::beginFrame() {
    if (!initialized_) return;
    
    // Clear render target
    float clearColor[4] = { clearColor_.r, clearColor_.g, clearColor_.b, clearColor_.a };
    context_->ClearRenderTargetView(renderTargetView_, clearColor);
}

void DirectXBackend::endFrame() {
    if (!initialized_) return;
    
    // Flush context
    context_->Flush();
}

void DirectXBackend::present() {
    if (!initialized_) return;
    
    // Present swap chain
    swapChain_->Present(1, 0);
}

void DirectXBackend::setViewport(int x, int y, int width, int height) {
    if (!initialized_) return;
    
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = static_cast<FLOAT>(x);
    viewport.TopLeftY = static_cast<FLOAT>(y);
    viewport.Width = static_cast<FLOAT>(width);
    viewport.Height = static_cast<FLOAT>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    
    context_->RSSetViewports(1, &viewport);
    
    viewportWidth_ = width;
    viewportHeight_ = height;
}

void DirectXBackend::setProjection(const Mat4& projection) {
    projection_ = projection;
}

void DirectXBackend::clear(const Color& color) {
    clearColor_ = color;
}

void DirectXBackend::executeDrawCommand(const DrawCommand& cmd) {
    switch (cmd.type) {
        case DrawCommandType::DrawRect: drawRect(cmd.rect, cmd.color); break;
        case DrawCommandType::DrawRoundedRect: drawRoundedRect(cmd.rect, cmd.color, cmd.cornerRadius); break;
        case DrawCommandType::DrawImage: drawTexture(cmd.texture, cmd.rect, cmd.color); break;
        case DrawCommandType::DrawLine: drawLine(cmd.rect.x, cmd.rect.y, cmd.rect.right(), cmd.rect.bottom(), cmd.color, cmd.thickness); break;
        case DrawCommandType::DrawCircle: drawCircle(cmd.rect.x + cmd.rect.width * 0.5f, cmd.rect.y + cmd.rect.height * 0.5f, std::min(cmd.rect.width, cmd.rect.height) * 0.5f, cmd.color, cmd.filled); break;
        case DrawCommandType::DrawText: drawText(cmd.text, cmd.rect.x, cmd.rect.y, cmd.color, cmd.fontFamily, cmd.fontSize); break;
        case DrawCommandType::SetClipRect: setClipRect(cmd.clipRect); break;
        case DrawCommandType::ClearClipRect: clearClipRect(); break;
        default: break;
    }
}

void DirectXBackend::drawRect(const Rect& rect, const Color& color) {
    if (!initialized_) return;
    
    // Set shader for color rendering
    context_->VSSetShader(vertexShader_, nullptr, 0);
    context_->PSSetShader(pixelShaderColor_, nullptr, 0);
    
    // Set constant buffer
    ConstantBuffer cb;
    cb.projection = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(projection_.m));
    
    DirectX::XMMATRIX transform = DirectX::XMMatrixTranslation(rect.x, rect.y, 0.0f) *
                                   DirectX::XMMatrixScaling(rect.width, rect.height, 1.0f);
    cb.transform = transform;
    
    context_->UpdateSubresource(constantBuffer_, 0, nullptr, &cb, 0, 0);
    context_->VSSetConstantBuffers(0, 1, &constantBuffer_);
    
    // Set color
    float colorData[4] = { color.r, color.g, color.b, color.a };
    context_->PSSetConstantBuffers(1, 1, &constantBuffer_);
    
    // Draw quad
    context_->IASetVertexBuffers(0, 1, &vertexBuffer_, &vertexStride_, &vertexOffset_);
    context_->IASetIndexBuffer(indexBuffer_, DXGI_FORMAT_R16_UINT, 0);
    context_->DrawIndexed(6, 0, 0);
}

void DirectXBackend::drawRoundedRect(const Rect& rect, const Color& color, float radius) {
    // For simplicity, draw as regular rect
    // In production, would use rounded rect shader
    drawRect(rect, color);
}

void DirectXBackend::drawTexture(TextureHandle texture, const Rect& rect, const Color& tint) {
    if (!initialized_ || texture == 0) return;
    
    auto it = textures_.find(texture);
    if (it == textures_.end()) return;
    
    // Set shader for texture rendering
    context_->VSSetShader(vertexShader_, nullptr, 0);
    context_->PSSetShader(pixelShaderTexture_, nullptr, 0);
    
    // Set texture
    context_->PSSetShaderResources(0, 1, &it->second.shaderResourceView);
    context_->PSSetSamplers(0, 1, &samplerState_);
    
    // Set constant buffer
    ConstantBuffer cb;
    cb.projection = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(projection_.m));
    
    DirectX::XMMATRIX transform = DirectX::XMMatrixTranslation(rect.x, rect.y, 0.0f) *
                                   DirectX::XMMatrixScaling(rect.width, rect.height, 1.0f);
    cb.transform = transform;
    
    context_->UpdateSubresource(constantBuffer_, 0, nullptr, &cb, 0, 0);
    context_->VSSetConstantBuffers(0, 1, &constantBuffer_);
    
    // Set tint color
    float tintData[4] = { tint.r, tint.g, tint.b, tint.a };
    context_->PSSetConstantBuffers(1, 1, &constantBuffer_);
    
    // Draw quad
    context_->IASetVertexBuffers(0, 1, &vertexBuffer_, &vertexStride_, &vertexOffset_);
    context_->IASetIndexBuffer(indexBuffer_, DXGI_FORMAT_R16_UINT, 0);
    context_->DrawIndexed(6, 0, 0);
}

void DirectXBackend::drawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness) {
    // Draw line as thin rectangle
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = std::sqrt(dx * dx + dy * dy);
    
    if (length < 0.001f) return;
    
    float angle = std::atan2(dy, dx);
    
    Rect rect(x1, y1 - thickness / 2, length, thickness);
    
    // Apply rotation in production
    drawRect(rect, color);
}

void DirectXBackend::drawCircle(float cx, float cy, float radius, const Color& color, bool filled) {
    // Draw circle using triangle fan approximation
    const int segments = 32;
    
    std::vector<DXVertex> vertices;
    vertices.reserve(segments + 2);

    // Center vertex
    DXVertex center = { cx, cy, 0.0f, 0.5f, 0.5f, color.r, color.g, color.b, color.a };
    vertices.push_back(center);

    // Edge vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = (i / static_cast<float>(segments)) * 2.0f * 3.14159f;
        float x = cx + std::cos(angle) * radius;
        float y = cy + std::sin(angle) * radius;

        DXVertex v = { x, y, 0.0f, 0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle),
                       color.r, color.g, color.b, color.a };
        vertices.push_back(v);
    }
    
    // Update vertex buffer and draw
    context_->UpdateSubresource(vertexBuffer_, 0, nullptr, vertices.data(), 0, 0);
    context_->IASetVertexBuffers(0, 1, &vertexBuffer_, &vertexStride_, &vertexOffset_);
    context_->Draw(static_cast<UINT>(vertices.size()), 0);
}

void DirectXBackend::drawText(const std::string& text, float x, float y, const Color& color,
                               const std::string& fontFamily, float fontSize) {
    // In production, would use DirectWrite for text rendering
    // For now, skip text rendering
}

void DirectXBackend::setClipRect(const Rect& rect) {
    if (!initialized_) return;
    
    D3D11_RECT scissorRect;
    scissorRect.left = static_cast<LONG>(rect.x);
    scissorRect.top = static_cast<LONG>(rect.y);
    scissorRect.right = static_cast<LONG>(rect.x + rect.width);
    scissorRect.bottom = static_cast<LONG>(rect.y + rect.height);
    
    context_->RSSetScissorRects(1, &scissorRect);
}

void DirectXBackend::clearClipRect() {
    if (!initialized_) return;
    
    D3D11_RECT scissorRect = { 0, 0, static_cast<LONG>(viewportWidth_), static_cast<LONG>(viewportHeight_) };
    context_->RSSetScissorRects(1, &scissorRect);
}

TextureHandle DirectXBackend::createTexture(int width, int height, const void* data) {
    if (!initialized_) return 0;
    
    TextureData texData;
    
    // Create texture
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = static_cast<UINT>(width);
    texDesc.Height = static_cast<UINT>(height);
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;
    initData.SysMemPitch = width * 4;
    
    HRESULT hr = device_->CreateTexture2D(&texDesc, &initData, &texData.resource);
    
    if (FAILED(hr)) {
        return 0;
    }
    
    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    
    hr = device_->CreateShaderResourceView(texData.resource, &srvDesc, &texData.shaderResourceView);
    
    if (FAILED(hr)) {
        texData.resource->Release();
        return 0;
    }
    
    TextureHandle handle = nextTextureHandle_++;
    textures_[handle] = texData;
    
    return handle;
}

void DirectXBackend::destroyTexture(TextureHandle texture) {
    if (!initialized_ || texture == 0) return;
    
    auto it = textures_.find(texture);
    if (it != textures_.end()) {
        if (it->second.resource) it->second.resource->Release();
        if (it->second.shaderResourceView) it->second.shaderResourceView->Release();
        textures_.erase(it);
    }
}

void DirectXBackend::updateTexture(TextureHandle texture, int width, int height, const void* data) {
    if (!initialized_ || texture == 0) return;
    
    auto it = textures_.find(texture);
    if (it == textures_.end()) return;
    
    context_->UpdateSubresource(it->second.resource, 0, nullptr, data, width * 4, 0);
}

bool DirectXBackend::isInitialized() const {
    return initialized_;
}

std::string DirectXBackend::getRendererInfo() const {
    return "DirectX 11 Backend";
}

void DirectXBackend::createShaders() {
    // Vertex shader
    const char* vertexShaderSource = R"(
        cbuffer ConstantBuffer : register(b0) {
            matrix projection;
            matrix transform;
        };
        
        cbuffer ColorBuffer : register(b1) {
            float4 color;
        };
        
        struct VS_INPUT {
            float4 pos : POSITION;
            float2 tex : TEXCOORD0;
            float4 col : COLOR0;
        };
        
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
            float4 col : COLOR0;
        };
        
        PS_INPUT VSMain(VS_INPUT input) {
            PS_INPUT output;
            output.pos = mul(input.pos, transform);
            output.pos = mul(output.pos, projection);
            output.tex = input.tex;
            output.col = input.col;
            return output;
        }
    )";
    
    // Pixel shader for color rendering
    const char* pixelShaderColorSource = R"(
        cbuffer ColorBuffer : register(b1) {
            float4 color;
        };
        
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
            float4 col : COLOR0;
        };
        
        float4 PSMain(PS_INPUT input) : SV_TARGET {
            return input.col * color;
        }
    )";
    
    // Pixel shader for texture rendering
    const char* pixelShaderTextureSource = R"(
        cbuffer ColorBuffer : register(b1) {
            float4 tint;
        };
        
        Texture2D tex : register(t0);
        SamplerState samp : register(s0);
        
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
            float4 col : COLOR0;
        };
        
        float4 PSMain(PS_INPUT input) : SV_TARGET {
            float4 texColor = tex.Sample(samp, input.tex);
            return texColor * input.col * tint;
        }
    )";
    
    // Compile and create shaders
    ID3DBlob* vertexBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    
    D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr,
               "VSMain", "vs_4_0", 0, 0, &vertexBlob, &errorBlob);
    
    device_->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
                                nullptr, &vertexShader_);
    
    // Create input layout
    D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    device_->CreateInputLayout(inputElements, 3, vertexBlob->GetBufferPointer(),
                               vertexBlob->GetBufferSize(), &inputLayout_);
    
    vertexBlob->Release();
    
    // Create pixel shaders
    ID3DBlob* pixelBlob = nullptr;
    
    D3DCompile(pixelShaderColorSource, strlen(pixelShaderColorSource), nullptr, nullptr, nullptr,
               "PSMain", "ps_4_0", 0, 0, &pixelBlob, &errorBlob);
    
    device_->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(),
                               nullptr, &pixelShaderColor_);
    
    pixelBlob->Release();
    
    D3DCompile(pixelShaderTextureSource, strlen(pixelShaderTextureSource), nullptr, nullptr, nullptr,
               "PSMain", "ps_4_0", 0, 0, &pixelBlob, &errorBlob);
    
    device_->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(),
                               nullptr, &pixelShaderTexture_);
    
    pixelBlob->Release();
    
    // Create constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(ConstantBuffer);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    
    device_->CreateBuffer(&cbDesc, nullptr, &constantBuffer_);
}

void DirectXBackend::createBuffers() {
    // Vertex buffer for quad
    DXVertex vertices[] = {
        { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f }
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;

    device_->CreateBuffer(&vbDesc, &vbData, &vertexBuffer_);

    vertexStride_ = sizeof(DXVertex);
    vertexOffset_ = 0;
    
    // Index buffer
    WORD indices[] = { 0, 1, 2, 0, 2, 3 };
    
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;
    
    device_->CreateBuffer(&ibDesc, &ibData, &indexBuffer_);
}

} // namespace NexusForge::UI

#endif // NEXUS_ENABLE_DX12
