#pragma once

#include <memory>
#include <vector>

typedef std::shared_ptr<class Material> MaterialPtr;

class Material
{
public:
	std::shared_ptr<class VertexShader> vertexShader;
	std::shared_ptr<class PixelShader> pixelShader;

	std::vector<std::shared_ptr<struct Texture2D>> textures;

	std::shared_ptr<class PipelineStateObject> pso;

	void BuildPSO(class D3D12GraphicsDevice& device);

	virtual void SetShaderParameters(class D3D12GraphicsDevice& device);
};

typedef std::shared_ptr<Material> MaterialPtr;

class NeuralTextureMaterial : public Material
{
public:
	virtual void SetShaderParameters(class D3D12GraphicsDevice& device) override;

	std::shared_ptr<class NeuralModel> model;
};