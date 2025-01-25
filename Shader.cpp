#include "Shader.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <iostream>
#include "Texture2D.h"
#include "Graphics.h"
#include "NeuralModel.h"
#include <DirectXMath.h>

#pragma comment(lib, "d3dcompiler.lib")

void Shader::Compile(D3D12GraphicsDevice& device)
{
	//compile shader
	std::wstring shaderFilePath = GetShaderFilePath();
	std::string shaderEntryPoint = GetShaderEntryPoint();
	std::string shaderTarget = GetShaderTarget();

	ID3D10Blob* errorBlob = nullptr;

	std::wstring shaderRoot = L"Shaders/";
	shaderFilePath = shaderRoot + shaderFilePath;

	std::vector<D3D_SHADER_MACRO> defines;
	GetDefines(defines);
	defines.push_back({ nullptr, nullptr });

	//include
	ID3DInclude* include = D3D_COMPILE_STANDARD_FILE_INCLUDE;

	UINT compileFlags = 0;
#ifdef _DEBUG
	//compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	HRESULT hr = D3DCompileFromFile(shaderFilePath.c_str(), defines.data(), include, shaderEntryPoint.c_str(), shaderTarget.c_str(), compileFlags, 0, &shaderBlob, &errorBlob);
	if (FAILED(hr))
	{
		std::cout << "Failed to compile shader" << std::endl;
		if (errorBlob)
		{
			std::cout << (char*)errorBlob->GetBufferPointer() << std::endl;
			errorBlob->Release();
		}
	}
}

std::vector<D3D12_INPUT_ELEMENT_DESC> VertexShader::GetInputLayout() const
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	return inputElementDescs;
}

void PixelShader::Compile(D3D12GraphicsDevice& device)
{
	Shader::Compile(device);
	CreateRootSignature(device);
	CreateConstantBuffer(device);
}

void PixelShader::CreateRootSignature(D3D12GraphicsDevice& device)
{
	using namespace std;
	vector<string> srvs, cbvs, uavs;
	GetSRVs(srvs);
	GetCBVs(cbvs);
	GetUAVs(uavs);

	size_t numRootParameters = srvs.size() + cbvs.size() + uavs.size();

	std::vector<CD3DX12_ROOT_PARAMETER> rootParameters(numRootParameters);
	std::vector<CD3DX12_DESCRIPTOR_RANGE> ranges(numRootParameters);

	int rootParameterIndex = 0;
	for (int i = 0; i < srvs.size(); i++)
	{
		ranges[rootParameterIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i);
		rootParameters[rootParameterIndex].InitAsDescriptorTable(1, &ranges[rootParameterIndex]);
		rootParameterIndex++;
	}

	for (int i = 0; i < cbvs.size(); i++)
	{
		ranges[rootParameterIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, i);
		rootParameters[rootParameterIndex].InitAsDescriptorTable(1, &ranges[rootParameterIndex]);
		rootParameterIndex++;
	}

	for (int i = 0; i < uavs.size(); i++)
	{
		ranges[rootParameterIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, i);
		rootParameters[rootParameterIndex].InitAsDescriptorTable(1, &ranges[rootParameterIndex]);
		rootParameterIndex++;
	}

	vector<D3D12_STATIC_SAMPLER_DESC> samplerDescs;
	GetSamplerStates(samplerDescs);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_0((UINT)numRootParameters, rootParameters.data(), (UINT)samplerDescs.size(), samplerDescs.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> error;
	if (D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error) != S_OK)
	{
		if (error)
		{
			std::cout << (char*)error->GetBufferPointer() << std::endl;
		}
	}


	CHECKHR(device.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void PixelShader::CreateConstantBuffer(D3D12GraphicsDevice& device)
{
	UINT cbSize = sizeof(RectConstantBuffer);

	D3D12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
	device.GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer.resource));

	//create constant buffer view
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constantBuffer.resource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cbSize;

	heapAllocator.Alloc(&constantBuffer.cpuHandle, &constantBuffer.gpuHandle);
	device.GetDevice()->CreateConstantBufferView(&cbvDesc, constantBuffer.cpuHandle);
}

void PixelShader::SetShaderParameters(D3D12GraphicsDevice& device, const std::vector<Texture2DPtr>& textures, const RectConstantBuffer& rectConstantBuffer)
{
	//set shader parameters
	ID3D12GraphicsCommandList* commandList = device.GetCommandList();

	commandList->SetGraphicsRootSignature(rootSignature.Get());

	int rootdescriptorIndex = 0;
	for (int i = 0; i < textures.size(); i++)
	{	
		commandList->SetGraphicsRootDescriptorTable(i, textures[i]->gpuHandle);
	}

	rootdescriptorIndex += (int)textures.size();

;
	//update constant buffer
	void* mappedBuffer;
	CHECKHR(constantBuffer.resource->Map(0, 0, &mappedBuffer));
	memcpy(mappedBuffer, &rectConstantBuffer, sizeof(RectConstantBuffer));
	constantBuffer.resource->Unmap(0, 0);

	commandList->SetGraphicsRootDescriptorTable(rootdescriptorIndex, constantBuffer.gpuHandle);

	//commandList->SetGraphicsRootConstantBufferView(ConstantBuffer, constantBuffer.resource->GetGPUVirtualAddress());
}

void NeuralPixelShader::SetShaderParameters(D3D12GraphicsDevice& device, const std::vector<Texture2DPtr>& textures, const RectConstantBuffer& rectConstantBuffer, const NeuralModelPtr& model)
{
	//Set Model Parameters
	ID3D12GraphicsCommandList* commandList = device.GetCommandList();

	//Set Shader Parameters
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	int rootdescriptorIndex = 0;
	//Set Textures
	for (int i = 0; i < textures.size(); i++)
	{
		commandList->SetGraphicsRootDescriptorTable(i, textures[i]->gpuHandle);
	}

	rootdescriptorIndex += (int)textures.size();

	if (model->weightBuffer == nullptr || model->biasBuffer == nullptr)
	{
		model->CreateBuffers(device);
	}

	//Set Model Parameters
	commandList->SetGraphicsRootDescriptorTable(rootdescriptorIndex++, model->weightBuffer->gpuHandle);
	commandList->SetGraphicsRootDescriptorTable(rootdescriptorIndex++, model->biasBuffer->gpuHandle);

	//Set rect constant buffer
	void* mappedBuffer;
	CHECKHR(constantBuffer.resource->Map(0, 0, &mappedBuffer));
	memcpy(mappedBuffer, &rectConstantBuffer, sizeof(RectConstantBuffer));
	constantBuffer.resource->Unmap(0, 0);

	commandList->SetGraphicsRootDescriptorTable(rootdescriptorIndex++, constantBuffer.gpuHandle);


}
