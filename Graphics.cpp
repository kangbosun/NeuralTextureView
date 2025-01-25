

#include "AppState.h"
#include <DirectXColors.h>
#include <iostream>
#include "d3dx12.h"
#include <d3dcompiler.h>
#include "Texture2D.h"
#include "Shader.h"
#include "Material.h"

//pix
#include <pix3.h>
#include <filesystem>

using namespace DirectX;

//load d3d12 library
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "WinPixEventRuntime.lib")

//heap allocator
DescriptorHeapAllocator heapAllocator;

D3D12GraphicsDevice::D3D12GraphicsDevice()
{
	std::cout << "D3D12GraphicsDevice created" << std::endl;
}

 D3D12GraphicsDevice::~D3D12GraphicsDevice()
{
	std::cout << "D3D12GraphicsDevice destroyed" << std::endl;
}



 //Macro to check for HRESULT for dx12 functions assert if failed and log location and reason
 //create error handler
#define CHECKHR(x) { HRESULT hr = x; if(FAILED(hr)) { std::cout << "Error: " << __FILE__ << ":" << __LINE__ << " HRESULT: " << hr << std::endl; assert(false); } }


 //simple texture drawing vertex shader class
 struct ScreenVertex
 {
	 XMFLOAT3 position;
	 XMFLOAT2 uv;
 };

 //ScreenRectIndexBuffer
 struct ScreenRectIndexBuffer
 {
	 //single instance of index buffer
	 static ScreenRectIndexBuffer& Get()
	 {
		 static ScreenRectIndexBuffer instance;
		 return instance;
	 }
	 //create index buffer
	 void Create(ID3D12Device* device)
	 {
		 //create index buffer if not already created
		 if (indexBuffer == nullptr)
		 {
			 //create index buffer
			 D3D12_HEAP_PROPERTIES heapProps = {};
			 heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			 D3D12_RESOURCE_DESC bufferDesc = {};
			 bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			 bufferDesc.Width = sizeof(indices);
			 bufferDesc.Height = 1;
			 bufferDesc.DepthOrArraySize = 1;
			 bufferDesc.MipLevels = 1;
			 bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
			 bufferDesc.SampleDesc.Count = 1;
			 bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			 bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			 CHECKHR(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, IID_PPV_ARGS(&indexBuffer)));
			 //copy index data to index buffer
			 void* mappedBuffer;
			 CHECKHR(indexBuffer->Map(0, 0, &mappedBuffer));
			 memcpy(mappedBuffer, indices, sizeof(indices));
			 indexBuffer->Unmap(0, 0);
			 //create index buffer view
			 ibv.BufferLocation = indexBuffer->GetGPUVirtualAddress();
			 ibv.SizeInBytes = sizeof(indices);
			 ibv.Format = DXGI_FORMAT_R16_UINT;
		 }
	 }
	 //release index buffer
	 void Release()
	 {
		 if (indexBuffer)
		 {
			 indexBuffer->Release();
			 indexBuffer = nullptr;
		 }
	 }
	 //delete copy constructor
	 ScreenRectIndexBuffer() = default;
	 ScreenRectIndexBuffer(const ScreenRectIndexBuffer&) = delete;
	 ScreenRectIndexBuffer& operator=(const ScreenRectIndexBuffer&) = delete;
	 ID3D12Resource* indexBuffer = nullptr;
	 D3D12_INDEX_BUFFER_VIEW ibv = {};
	 USHORT indices[6] = { 0, 1, 2, 0, 2, 3 };
 };

 // Create a vertex buffer for a full screen triangle
 struct Vertex
 {
	 XMFLOAT3 position;
	 XMFLOAT2 uv;
 };

 //full screen rect vertex buffer class
 struct FullScreenRectVertexBuffer
 {
	 //single instance of full screen rect
	 static FullScreenRectVertexBuffer& Get()
	 {
		 static FullScreenRectVertexBuffer instance;
		 return instance;
	 }
	 //create vertex buffer
	 void Create(ID3D12Device* device)
	 {
		 //create vertex buffer if not already created
		 if (vertexBuffer == nullptr)
		 {
			 //create vertex buffer
			 D3D12_HEAP_PROPERTIES heapProps = {};
			 heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			 D3D12_RESOURCE_DESC bufferDesc = {};
			 bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			 bufferDesc.Width = sizeof(vertices);
			 bufferDesc.Height = 1;
			 bufferDesc.DepthOrArraySize = 1;
			 bufferDesc.MipLevels = 1;
			 bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
			 bufferDesc.SampleDesc.Count = 1;
			 bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			 bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			 CHECKHR(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, IID_PPV_ARGS(&vertexBuffer)));
			 //copy vertex data to vertex buffer
			 void* mappedBuffer;
			 CHECKHR(vertexBuffer->Map(0, 0, &mappedBuffer));
			 memcpy(mappedBuffer, vertices, sizeof(vertices));
			 vertexBuffer->Unmap(0, 0);
			 //create vertex buffer view
			 vbv.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
			 vbv.StrideInBytes = sizeof(Vertex);
			 vbv.SizeInBytes = sizeof(vertices);
		 }
	 }
	 //release vertex buffer
	 void Release()
	 {
		 if (vertexBuffer)
		 {
			 vertexBuffer->Release();
			 vertexBuffer = nullptr;
		 }
	 }
	 //delete copy constructor
	 FullScreenRectVertexBuffer() = default;
	 FullScreenRectVertexBuffer(const FullScreenRectVertexBuffer&) = delete;
	 FullScreenRectVertexBuffer& operator=(const FullScreenRectVertexBuffer&) = delete;
	 ID3D12Resource* vertexBuffer = nullptr;
	 D3D12_VERTEX_BUFFER_VIEW vbv = {};
	 ScreenVertex vertices[3] =
	 {
		 { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		 { XMFLOAT3(3.0f,  1.0f, 0.0f), XMFLOAT2(2.0f, 0.0f) },
		 { XMFLOAT3(-1.0f, -3.0f, 0.0f), XMFLOAT2(0.0f, 2.0f) },
	 };
 };

 //function for get device remove reason
 std::string GetDeviceRemovedReason(HRESULT hr)
 {
	 switch (hr)
	 {
	 case DXGI_ERROR_DEVICE_HUNG:
		 return "DXGI_ERROR_DEVICE_HUNG: The GPU device instance hung";
	 case DXGI_ERROR_DEVICE_REMOVED:
		 return "DXGI_ERROR_DEVICE_REMOVED: The GPU device instance has been removed";
	 case DXGI_ERROR_DEVICE_RESET:
		 return "DXGI_ERROR_DEVICE_RESET: The GPU device instance has been reset";
	 case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
		 return "DXGI_ERROR_DRIVER_INTERNAL_ERROR: The GPU driver encountered a problem and was put into the device removed state";
	 case DXGI_ERROR_INVALID_CALL:
		 return "DXGI_ERROR_INVALID_CALL: The application made a call that was invalid";
	 case S_OK:
		 return "S_OK: The device is removed";
	 default:
		 return "Unknown error";
	 }
 }

 void D3D12GraphicsDevice::Initialize(HWND hwnd, int width, int height)
 {
	using namespace Microsoft::WRL;
	UINT dxgiFactoryFlags = 0;
	 //enable debug layer
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController0)))) {
		debugController0->EnableDebugLayer();
	}

	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	
	//load pix
	for (std::string& arg : gAppState.arguments)
	{
		if (arg == "-pix")
		{
			HMODULE hModule = LoadLibrary(L"WinPixEventRuntime.dll");
			if (hModule == NULL)
			{
				std::cout << "Failed to load WinPixEventRuntime.dll" << std::endl;
			}

			//auto enable pix
			if (hModule)
			{
				PIXLoadLatestWinPixGpuCapturerLibrary();
			}
		}
	}
	
	//create device
	 IDXGIFactory4* factory;
	 CHECKHR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
	 IDXGIAdapter1* adapter;
	 CHECKHR(factory->EnumAdapters1(0, &adapter));
	 CHECKHR(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

	 //create command queue
	 D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	 queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	 queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	 CHECKHR(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
	 //create swap chain
	 DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	 swapChainDesc.BufferCount = 2;
	 swapChainDesc.Width = width;
	 swapChainDesc.Height = height;
	 swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	 swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	 swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	 swapChainDesc.SampleDesc.Count = 1;
	 IDXGISwapChain1* tempSwapChain;
	 CHECKHR(factory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, 0, 0, &tempSwapChain));
	 CHECKHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain)));
	 tempSwapChain->Release();
	 factory->Release();

	 //create descriptor heap
	 D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	 rtvHeapDesc.NumDescriptors = 2;
	 rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	 rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	 CHECKHR(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
	 rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	 //create shader resource view heap
	 D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	 srvHeapDesc.NumDescriptors = 128;
	 srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	 srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	 CHECKHR(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));

	 //create shader resource view heap allocator
	 heapAllocator.Create(device, srvHeap);

	 //Create frame resources
	 CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	 for (UINT i = 0; i < 2; i++)
	 {
		 CHECKHR(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
		 device->CreateRenderTargetView(renderTargets[i], 0, rtvHandle);
		 rtvHandle.Offset(1, rtvDescriptorSize);
	 }

	 //create command allocator
	 CHECKHR(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
	 //create command list
	 CHECKHR(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, 0, IID_PPV_ARGS(&commandList)));
	 commandList->Close();

	 //create fence
	 CHECKHR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	 //create event handle
	 fenceEvent = CreateEvent(0, 0, 0, 0);
	 fenceValue = 1;


	 //create full screen rect vertex buffer	
	 FullScreenRectVertexBuffer::Get().Create(device);

	 //create full screen rect index buffer
	 ScreenRectIndexBuffer::Get().Create(device);

	 //Set back buffer size to global app state
	 gAppState.backBufferWidth = width;
	 gAppState.backBufferHeight = height;

	 //create default textures
	 static Texture2D::TextureCreateParams WhiteTextureParams;
	 WhiteTextureParams.data = { 255, 255, 255, 255 };
	 WhiteTextureParams.width = 1;
	 WhiteTextureParams.height = 1;
	 WhiteTextureParams.mipLevels = 1;
	 WhiteTextureParams.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	 Texture2D::WhiteTexture = Texture2D::CreateFromMemory(*this, WhiteTextureParams);

	 static Texture2D::TextureCreateParams BlackTextureParams;
	 BlackTextureParams.data = { 0, 0, 0, 255 };
	 BlackTextureParams.width = 1;
	 BlackTextureParams.height = 1;
	 BlackTextureParams.mipLevels = 1;
	 BlackTextureParams.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	 Texture2D::BlackTexture = Texture2D::CreateFromMemory(*this, BlackTextureParams);

	 D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
	 device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

	 UINT maxTextureSize = (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3) ? 16384 : 8192;
	 OutputDebugString((std::wstring(L"최대 텍스처 크기: ") + std::to_wstring(maxTextureSize) + L"\n").c_str());

}

//pre render
void D3D12GraphicsDevice::PreRender()
{
	//Scoped PIX event
	//PIXScopedEvent(commandList, PIX_COLOR(1, 0, 0), "PreRender");

	//wait for previous frame
	WaitForPreviousFrame();

	UINT backBufferIdx = swapChain->GetCurrentBackBufferIndex();
	commandAllocator->Reset();

	//resource transition barrier
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = renderTargets[backBufferIdx];
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	commandList->Reset(commandAllocator, 0);
	commandList->ResourceBarrier(1, &barrier);


	//clear render target
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), backBufferIdx, rtvDescriptorSize);
	float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	commandList->OMSetRenderTargets(1, &rtvHandle, 0, 0);
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, 0);

	//set descriptor heap
	ID3D12DescriptorHeap* heaps[] = { srvHeap };
	commandList->SetDescriptorHeaps(1, heaps);
}

void D3D12GraphicsDevice::Render(float deltaTime)
{
	//Scoped PIX event
	PIXScopedEvent(commandList, PIX_COLOR(0, 1, 0), "Render");

	//pending commands to render queue
	for (auto& command : pendingCommands)
	{
		renderQueue.push(command);
	}
	pendingCommands.clear();

	//execute all render queue
	while (!renderQueue.empty())
	{
		RenderCommand& command = renderQueue.front();
		command(*this);
		renderQueue.pop();
	}


}

void D3D12GraphicsDevice::Present()
{
	//PIX
	//PIXScopedEvent(commandList, 0, "Present");

	UINT backBufferIdx = swapChain->GetCurrentBackBufferIndex();

	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = renderTargets[backBufferIdx];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

		commandList->ResourceBarrier(1, &barrier);
	}
	
	commandList->Close();

	//execute command list
	ID3D12CommandList* commandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(1, commandLists);

	// Present
	HRESULT hr = swapChain->Present(vsync, 0);   // Present with vsync
	if (hr == DXGI_ERROR_DEVICE_REMOVED)
	{
		std::cout << "Device removed" << std::endl;
		std::cout << GetDeviceRemovedReason(device->GetDeviceRemovedReason()) << std::endl;
		//raise exception
		throw std::exception("Device removed");
	}

	bOccluded = hr == DXGI_STATUS_OCCLUDED;
}

//post render
void D3D12GraphicsDevice::PostRender()
{
	//Scoped PIX event
	//PIXScopedEvent(commandList, PIX_COLOR(0, 0, 1), "PostRender");
	//wait for previous frame
	WaitForPreviousFrame();
}

void D3D12GraphicsDevice::SetVsync(UINT vsync)
{
	this->vsync = vsync;
}

void D3D12GraphicsDevice::Resize(int width, int height)
{
	//wait for previous frame
	WaitForPreviousFrame();

	//release render targets
	for (int i = 0; i < 2; i++)
	{
		renderTargets[i]->Release();
	}
	//resize swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChain->GetDesc1(&swapChainDesc);
	CHECKHR(swapChain->ResizeBuffers(2, width, height, swapChainDesc.Format, swapChainDesc.Flags));
	//create new render targets
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < 2; i++)
	{
		CHECKHR(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
		device->CreateRenderTargetView(renderTargets[i], 0, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	//Set back buffer size to global app state
	gAppState.backBufferWidth = width;
	gAppState.backBufferHeight = height;
}

void D3D12GraphicsDevice::Cleanup()
{
	if (device) device->Release();
	if (commandQueue) commandQueue->Release();
	if (commandAllocator) commandAllocator->Release();
	if (commandList) commandList->Release();
	if (swapChain) swapChain->Release();
	if (rtvHeap) rtvHeap->Release();
	if (srvHeap) srvHeap->Release();
	for (int i = 0; i < 2; ++i)
	{
		if (renderTargets[i]) renderTargets[i]->Release();
	}
	if (fence) fence->Release();
}

void D3D12GraphicsDevice::WaitForPreviousFrame()
{
	//Scoped PIX event
	//PIXScopedEvent(commandList, PIX_COLOR(1, 0, 0), "WaitForPreviousFrame");
	// Signal and increment the fence value.
	//store current fence value
	UINT64 currentFenceValue = fenceValue;
	CHECKHR(commandQueue->Signal(fence, currentFenceValue));
	// Wait until the previous frame is finished.
	fenceValue++;
	if (fence->GetCompletedValue() < currentFenceValue)
	{
		CHECKHR(fence->SetEventOnCompletion(currentFenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void D3D12GraphicsDevice::DrawFullScreenRect(const std::shared_ptr<Material>& material)
{
	//Scoped PIX event
	PIXScopedEvent(commandList, PIX_COLOR(0, 0, 1), "DrawFullScreenRect");

	//Get or Create Pipeline State Object for Full Screen Rect
	if (material->pso == nullptr)
	{	
		material->BuildPSO(*this);
	}

	//Set Buffers
	commandList->IASetVertexBuffers(0, 1, &FullScreenRectVertexBuffer::Get().vbv);
	commandList->IASetIndexBuffer(&ScreenRectIndexBuffer::Get().ibv);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Set Pipeline State Object
	commandList->SetPipelineState(material->pso->PSO);

	//Set Shader Resources
	material->SetShaderParameters(*this);

	//Set Viewport and Scissor Rect
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)gAppState.backBufferWidth, (float)gAppState.backBufferHeight, 0.0f, 1.0f };
	D3D12_RECT scissorRect = { 0, 0, gAppState.backBufferWidth, gAppState.backBufferHeight };
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	//Draw
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void D3D12GraphicsDevice::AddRenderCommand(const RenderCommand& command)
{
	renderQueue.push(command);
}





