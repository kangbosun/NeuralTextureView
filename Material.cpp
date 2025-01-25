#include "Material.h"
#include <d3dcompiler.h>
#include <d3d12.h>
#include <iostream>
#include "Graphics.h"
#include "Shader.h"
#include "Texture2D.h"
#include "d3dx12.h"

void Material::BuildPSO(D3D12GraphicsDevice& device)
{
	//create pso
	pso = std::make_shared<PipelineStateObject>();

	//create vertex shader
	ID3DBlob* vsBlob = vertexShader->GetShaderBlob().Get();
	D3D12_SHADER_BYTECODE vsBytecode = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	//create pixel shader
	ID3DBlob* psBlob = pixelShader->GetShaderBlob().Get();
	D3D12_SHADER_BYTECODE psBytecode = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

	auto inputElementDescs = vertexShader->GetInputLayout();

	//create pipeline state object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs.data(), (UINT32)inputElementDescs.size() };
	psoDesc.pRootSignature = pixelShader->GetRootSignature().Get();
	psoDesc.VS = vsBytecode;
	psoDesc.PS = psBytecode;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	HRESULT hr = device.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso->PSO));
	if (hr != S_OK)
	{
		std::cout << "Failed to create PSO " << hr << std::endl;

		assert(false);
	}
}

void Material::SetShaderParameters(D3D12GraphicsDevice& device)
{
	//set shader parameters
	pixelShader->SetShaderParameters(device, textures, device.rectConstantBuffer);
}

void NeuralTextureMaterial::SetShaderParameters(D3D12GraphicsDevice& device)
{
	auto neuralpixelShader = std::dynamic_pointer_cast<NeuralPixelShader>(pixelShader);

	neuralpixelShader->SetShaderParameters(device, textures, device.rectConstantBuffer, model);
}
