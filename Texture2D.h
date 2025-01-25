#pragma once
#include <wrl\client.h>
#include <d3d12.h>
#include <memory>
#include <string>
#include <vector>


// Get common simple names from DXGI format (ex. RGBA16, RGB)
std::wstring GetFormatString(DXGI_FORMAT format);

//simple texture desc struct
struct TextureDesc
{
	std::wstring Format;
	UINT64 Width = 0;
	UINT Height = 0;
	UINT16 MipLevels = 1;
};

typedef std::shared_ptr<struct Texture2D> Texture2DPtr;

// Texture2D struct
struct Texture2D
{
public:
	Microsoft::WRL::ComPtr<ID3D12Resource> texture;
	std::unique_ptr<uint8_t[]> decodedData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;

	//simple texture descriptor
	TextureDesc desc;

	//descriptor handle
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;

	//upload buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

	//texture name
	std::wstring name;

	//byte sizes data for debug
	UINT uncompressedByteSize = 0;
	UINT compressedByteSize = 0;

	//GetDesc
	TextureDesc GetDesc() const
	{
		return desc;
	}

	//Cleanup subresource data after texture created
	void CleanupCPUMemory()
	{
		if (decodedData)
		{
			decodedData.reset();
		}
		if (!subresources.empty())
		{
			subresources.clear();
		}
	}

	//release texture
	void Release();

	// Add Create from file static method return ComPtr
	static Texture2DPtr CreateFromFile(class D3D12GraphicsDevice& device, const wchar_t* filename);

	struct TextureCreateParams
	{
		std::vector<uint8_t> data;
		size_t width;
		size_t height;
		size_t mipLevels;
		DXGI_FORMAT format;
	};

	// create from memory
	static Texture2DPtr CreateFromMemory(class D3D12GraphicsDevice& device, const TextureCreateParams& Params);

	//Create from DDS
	static Texture2DPtr CreateFromDDS(class D3D12GraphicsDevice& device, const wchar_t* filename);

	//default textures
	static Texture2DPtr WhiteTexture;
	static Texture2DPtr BlackTexture;
};

