#pragma once

#include <memory>
#include <vector>
#include <string>

typedef std::shared_ptr<class Material> MaterialPtr;
typedef std::shared_ptr<struct Texture2D> Texture2DPtr;

struct TextureSlot
{
	Texture2DPtr texture;

	std::string name;

	bool bEnable = true;
};

class Material
{
public:
	std::shared_ptr<class VertexShader> vertexShader;
	std::shared_ptr<class PixelShader> pixelShader;

	std::shared_ptr<struct PipelineStateObject> pso;

	void SetTexture(int slotIndex, Texture2DPtr texture, const std::string& name = "");

	void BuildPSO(class D3D12GraphicsDevice& device);

	std::vector<TextureSlot>& GetTextureSlots() { return textureSlots; }

	virtual void SetShaderParameters(class D3D12GraphicsDevice& device);

protected:
	void CollectTextures(std::vector<Texture2DPtr>& textures);

	std::vector<TextureSlot> textureSlots;
};

typedef std::shared_ptr<Material> MaterialPtr;


class NeuralTextureMaterial : public Material
{
public:
	virtual void SetShaderParameters(class D3D12GraphicsDevice& device) override;

	std::shared_ptr<class NeuralModel> model;
};