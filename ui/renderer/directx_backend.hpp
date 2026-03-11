// ui/renderer/directx_backend.hpp
#pragma once

#include "render_engine.hpp"

#ifdef NEXUS_ENABLE_DX12

#include <d3d12.h>
#include <dxgi1_4.h>
#include <vector>
#include <string>

namespace NexusForge::UI {

// DirectX 12 render backend
class DirectXBackend : public RenderBackend {
public:
    DirectXBackend();
    ~DirectXBackend() override;

    bool initialize() override;
    void shutdown() override;

    void setViewport(int x, int y, int width, int height) override;
    void clear(const Color& color) override;
    void present() override;

    void executeDrawCommand(const DrawCommand& cmd) override;

    // DirectX-specific
    ID3D12Device* getDevice() const { return device_; }
    ID3D12CommandQueue* getCommandQueue() const { return commandQueue_; }
    ID3D12RootSignature* getRootSignature() const { return rootSignature_; }
    ID3D12PipelineState* getPipelineState() const { return pipelineState_; }

private:
    ID3D12Device* device_ = nullptr;
    ID3D12CommandQueue* commandQueue_ = nullptr;
    ID3D12GraphicsCommandList* commandList_ = nullptr;
    ID3D12RootSignature* rootSignature_ = nullptr;
    ID3D12PipelineState* pipelineState_ = nullptr;

    IDXGISwapChain3* swapChain_ = nullptr;
    std::vector<ID3D12Resource*> renderTargets_;

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;

    bool createDevice();
    bool createCommandQueue();
    bool createSwapChain(void* window);
    bool createRenderTargets();
    bool createRootSignature();
    bool createPipelineState();
    bool createCommandList();
    bool createDescriptorHeaps();

    void waitForPreviousFrame();
    void moveToNextFrame();

    void renderRect(const DrawCommand& cmd);
    void renderRoundedRect(const DrawCommand& cmd);
    void renderCircle(const DrawCommand& cmd);
    void renderLine(const DrawCommand& cmd);
    void renderTriangle(const DrawCommand& cmd);
    void renderText(const DrawCommand& cmd);
    void renderImage(const DrawCommand& cmd);
};

} // namespace NexusForge::UI

#endif // NEXUS_ENABLE_DX12
