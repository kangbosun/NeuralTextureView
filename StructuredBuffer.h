#pragma once
#include <wrl\client.h>
#include <d3d12.h>
#include <memory>

class StructuredBuffer
{
public:
	Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> bufferUpload;

	// heap handles
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;

	void Initialize(class D3D12GraphicsDevice& device, void* data, size_t elementSize, size_t elementCount);

	void Bind(class D3D12GraphicsDevice& device, UINT rootParameterIndex);
};

typedef std::shared_ptr<StructuredBuffer> StructuredBufferPtr;

