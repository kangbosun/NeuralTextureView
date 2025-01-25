#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <cassert>
#include <vector>
#include <memory>
#include <wrl\client.h>
#include <string>
#include <functional>
#include <queue>

//Macro to check for HRESULT for dx12 functions assert if failed and log location and reason
//create error handler
#define CHECKHR(x) { HRESULT hr = x; if(FAILED(hr)) { std::cout << "Error: " << __FILE__ << ":" << __LINE__ << " HRESULT: " << hr << std::endl; assert(false); } }

// Simple free list based allocator
struct DescriptorHeapAllocator
{
    ID3D12DescriptorHeap* Heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT                        HeapHandleIncrement;
    std::vector<int>               FreeIndices;

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
    {
        //assert(Heap == nullptr && FreeIndices.empty());
        Heap = heap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
        HeapType = desc.Type;
        HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
        HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
        HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
        FreeIndices.reserve((int)desc.NumDescriptors);
        for (int n = desc.NumDescriptors; n > 0; n--)
            FreeIndices.push_back(n);
    }
    void Destroy()
    {
        Heap = nullptr;
        FreeIndices.clear();
    }
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        assert(FreeIndices.size() > 0);
        int idx = FreeIndices.back();
        FreeIndices.pop_back();
        out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
        out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
    }
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
    {
        int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
        int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
        assert(cpu_idx == gpu_idx);
        FreeIndices.push_back(cpu_idx);
    }
};

struct TextureSet
{
	std::shared_ptr<struct Texture2D> albedoTexture;
	std::shared_ptr<struct Texture2D> normalTexture;
	std::shared_ptr<struct Texture2D> AOTexture;
	std::shared_ptr<struct Texture2D> roughnessTexture;
	std::shared_ptr<struct Texture2D> displacementTexture;
};

struct RectConstantBuffer
{
	float scale = 1.f;
	float intensity = 2.5f;
	float lightpos = 0.f;
	float metalness = 0.0f;

	//padding 256 byte alignment
	float padding[60];
};

// type def for render command created by lamda
class D3D12GraphicsDevice;
typedef std::function<void(D3D12GraphicsDevice&)> RenderCommand;

class D3D12GraphicsDevice
{
public:
    D3D12GraphicsDevice();
    ~D3D12GraphicsDevice();
    
    //initialize d3d12 with back buffer
    void Initialize(HWND hwnd, int width, int height);
    
    void PreRender();

    //render frame
    void Render(float deltaTime);

	void Present();

    void PostRender();

    void SetVsync(UINT vsync);
    UINT GetVsync() const { return vsync; }

    // Add Resize method
    void Resize(int width, int height);

    // Add Cleanup method
    void Cleanup();

    bool IsSwapChainOccluded() const { return bOccluded; }

	void WaitForPreviousFrame();

    // Add DrawFullScreenRect method
    void DrawFullScreenRect(const std::shared_ptr<class Material>& material);

	// Add RenderCommand method
	void AddRenderCommand(const RenderCommand& command);

	RectConstantBuffer rectConstantBuffer;

public:
    //getter for device
    ID3D12Device* GetDevice() const { return device; }
    ID3D12DescriptorHeap* GetRtvHeap() const { return rtvHeap; }
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList; }
    ID3D12CommandQueue* GetCommandQueue() const { return commandQueue; }
    ID3D12DescriptorHeap* GetSrvHeap() const { return srvHeap; }

private:
    //D3D12 State
    ID3D12Device* device;
    ID3D12CommandQueue* commandQueue;
    ID3D12CommandAllocator* commandAllocator;
    ID3D12GraphicsCommandList* commandList;
    IDXGISwapChain3* swapChain;

    ID3D12DescriptorHeap* rtvHeap;
    ID3D12DescriptorHeap* srvHeap;
    ID3D12Resource* renderTargets[2];
    UINT rtvDescriptorSize;
    ID3D12Fence* fence;

	//debug layer
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController0;
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;

    //fence value
    UINT64 fenceValue = 0;
	HANDLE fenceEvent;

    UINT vsync = true;
    
    //swap chain occluded
    bool bOccluded = false;

	//thread safe queue for render command
	std::queue<RenderCommand> renderQueue;

	//pending render commands
	std::vector<RenderCommand> pendingCommands;
};

//getter  global heap allocator
extern DescriptorHeapAllocator heapAllocator;

//Pipeline state object struct
struct PipelineStateObject
{
	ID3D12PipelineState* PSO = nullptr;
	D3D12_RASTERIZER_DESC RasterizerState;
	D3D12_BLEND_DESC BlendState;
	D3D12_DEPTH_STENCIL_DESC DepthStencilState;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
	DXGI_FORMAT RTVFormat;
	DXGI_FORMAT DSVFormat;
	UINT SampleCount = 1;
	UINT SampleQuality = 0;
	UINT NodeMask = 0;
	D3D12_CACHED_PIPELINE_STATE CachedPSO;
	D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc;
	D3D12_PIPELINE_STATE_FLAGS Flags;
	ID3D12RootSignature* RootSignature = nullptr;

	ID3D12Resource* ConstantBuffer = nullptr;
	//cpu descriptor handle
	D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferHandleCPU;
	//gpu descriptor handle
	D3D12_GPU_DESCRIPTOR_HANDLE ConstantBufferHandleGPU;

	//destructor
	~PipelineStateObject()
	{
		if (PSO)
		{
			PSO->Release();
			PSO = nullptr;
		}
		if (RootSignature)
		{
			RootSignature->Release();
			RootSignature = nullptr;
		}
	}
};





