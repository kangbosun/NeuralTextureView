#include "StructuredBuffer.h"
#include "d3dx12.h"
#include "Graphics.h"
#include <iostream>

void StructuredBuffer::Initialize(D3D12GraphicsDevice& device, void* data, size_t elementSize, size_t elementCount)
{
	size_t bufferSize = elementSize * elementCount;

	//create default heap
	D3D12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	device.GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, 0, IID_PPV_ARGS(&bufferResource));

	//create upload heap
	D3D12_HEAP_PROPERTIES heapPropsUpload(D3D12_HEAP_TYPE_UPLOAD);
	device.GetDevice()->CreateCommittedResource(&heapPropsUpload, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, IID_PPV_ARGS(&bufferUpload));

	//copy data
	void* mappedBuffer;
	CHECKHR(bufferUpload->Map(0, 0, &mappedBuffer));
	memcpy(mappedBuffer, data, bufferSize);
	bufferUpload->Unmap(0, 0);

	//copy data to default heap
	device.GetCommandList()->CopyResource(bufferResource.Get(), bufferUpload.Get());

	heapAllocator.Alloc(&cpuHandle, &gpuHandle);

	//Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = (UINT)elementCount;
	srvDesc.Buffer.StructureByteStride = (UINT)elementSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device.GetDevice()->CreateShaderResourceView(bufferResource.Get(), &srvDesc, cpuHandle);
}

void StructuredBuffer::Bind(D3D12GraphicsDevice& device, UINT rootParameterIndex)
{
	ID3D12GraphicsCommandList* commandList = device.GetCommandList();
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, gpuHandle);
}
