#pragma once
#include <unordered_map>
#include <string>
#include <wrl\client.h>
#include <d3d12.h>
#include <memory>

class Shader
{
	friend class ShaderMap;
public :
	virtual std::wstring GetShaderFilePath() const = 0;
	virtual std::string GetShaderEntryPoint() const = 0;
	virtual std::string GetShaderTarget() const = 0;

	virtual void GetSRVs(std::vector<std::string>& outSRVs) {}
	virtual void GetCBVs(std::vector<std::string>& outCBVs) {}
	virtual void GetUAVs(std::vector<std::string>& outUAVs) {}

	virtual void GetSamplerStates(std::vector<D3D12_STATIC_SAMPLER_DESC>& outSamplerStates) {}

	virtual ~Shader()
	{
	}

	void Initialize(class D3D12GraphicsDevice& device)
	{
		Compile(device);
	}

	Microsoft::WRL::ComPtr<ID3DBlob> GetShaderBlob() const
	{
		return shaderBlob;
	}

	virtual void GetDefines(std::vector<D3D_SHADER_MACRO>& defines) const
	{
	}

protected:
	virtual void Compile(class D3D12GraphicsDevice& device);

	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
};

class VertexShader : public Shader
{
public:
	std::wstring GetShaderFilePath() const override
	{
		return L"VertexShader.hlsl";
	}
	std::string GetShaderEntryPoint() const override
	{
		return "VSMain";
	}
	std::string GetShaderTarget() const override
	{
		return "vs_5_0";
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout() const;
};

class PixelShader : public Shader
{
public:
	std::wstring GetShaderFilePath() const override
	{
		return L"PixelShader.hlsl";
	}
	std::string GetShaderEntryPoint() const override
	{
		return "PSMain";
	}
	std::string GetShaderTarget() const override
	{
		return "ps_5_0";
	}

	virtual void GetSRVs(std::vector<std::string>& outSRVs) override
	{
		outSRVs.push_back("AlbeoTexture");
		outSRVs.push_back("NormalsTexture");
		outSRVs.push_back("AOTexture");
		outSRVs.push_back("RoughnessTexture");
	}

	virtual void GetCBVs(std::vector<std::string>& outCBVs) override
	{
		outCBVs.push_back("ConstantBuffer");
	}

	virtual void GetSamplerStates(std::vector<D3D12_STATIC_SAMPLER_DESC>& outSamplerStates) override
	{
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		outSamplerStates.push_back(samplerDesc);
		samplerDesc.ShaderRegister = 1;

		outSamplerStates.push_back(samplerDesc);
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const
	{
		return rootSignature;
	}

	typedef std::shared_ptr<struct Texture2D> Texture2DPtr;
	void SetShaderParameters(class D3D12GraphicsDevice& device, const std::vector<Texture2DPtr>& textures, const struct RectConstantBuffer& rectConstantBuffer);

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob> signature;

	//constant buffer
	struct {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	} constantBuffer;


	virtual void Compile(D3D12GraphicsDevice& device) override;

	void CreateRootSignature(class D3D12GraphicsDevice& device);

	virtual void CreateConstantBuffer(class D3D12GraphicsDevice& device);
};

class NeuralPixelShader : public PixelShader
{
protected:
	void GetDefines(std::vector<D3D_SHADER_MACRO>& defines) const override
	{
		defines.push_back({ "USE_NEURAL_TEXTURES", "1" });
	}

	void GetSRVs(std::vector<std::string>& outSRVs) override
	{
		outSRVs.push_back("FeatureGrid0");
		outSRVs.push_back("FeatureGrid1");
		outSRVs.push_back("FeatureGrid2");
		outSRVs.push_back("FeatureGrid3");
		outSRVs.push_back("WeightBuffer");
		outSRVs.push_back("BiasBuffer");
	}

	void GetCBVs(std::vector<std::string>& outCBVs) override
	{
		outCBVs.push_back("ConstantBuffer");
	}

	void GetSamplerStates(std::vector<D3D12_STATIC_SAMPLER_DESC>& outSamplerStates) override
	{
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		//samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		outSamplerStates.push_back(samplerDesc);
	}

public:
	typedef std::shared_ptr<class NeuralModel> NeuralModelPtr;
	void SetShaderParameters(class D3D12GraphicsDevice& device, const std::vector<Texture2DPtr>& textures, const struct RectConstantBuffer& rectConstantBuffer, const NeuralModelPtr& model);
};


//Lightweight version NeuralPixelShader
class NeuralPixelShaderLight : public NeuralPixelShader
{
protected:
	void GetDefines(std::vector<D3D_SHADER_MACRO>& defines) const override
	{
		NeuralPixelShader::GetDefines(defines);

		defines.push_back({ "LIGHT_WEIGHT_NN", "1" });
	}
};


class ShaderMap
{

	//Singleton
public:
	static ShaderMap& Get()
	{
		static ShaderMap instance;
		return instance;
	}
	
	~ShaderMap() {}
	//Add shader
	void AddShader(const std::string& name, const std::shared_ptr<Shader>& shader)
	{
		shaders[name] = shader;
	}
	//Get shader
	template<typename T>
	std::shared_ptr<T> GetShader(class D3D12GraphicsDevice& device, const std::string& name)
	{
		//found
		if (shaders.find(name) != shaders.end())
		{
			return std::dynamic_pointer_cast<T>(shaders[name]);
		}

		auto shader = std::make_shared<T>();
		shader->Initialize(device);
		shaders[name] = shader;

		return shader;
	}
private:
	ShaderMap() {}

	std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
};

