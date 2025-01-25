
#include "Texture2D.h"
#include "Graphics.h"
#include "d3dx12.h"
#include <filesystem>
#include <string>
#include "ThirdParty/WICTextureLoader12.h"
#include "ThirdParty/DDSTextureLoader12.h"
#include <iostream>

void Texture2D::Release()
{
	if (texture)
	{
		texture.Reset();
	}
	if (uploadBuffer)
	{
		uploadBuffer.Reset();
	}

	//free descriptor handle
	if (cpuHandle.ptr)
	{
		heapAllocator.Free(cpuHandle, gpuHandle);
		cpuHandle.ptr = 0;
		gpuHandle.ptr = 0;
	}

	CleanupCPUMemory();
}

std::shared_ptr<Texture2D> Texture2D::CreateFromFile(D3D12GraphicsDevice& device, const wchar_t* filename)
{
	//filename to fullpath
	wchar_t fullpath[MAX_PATH];
	GetFullPathName(filename, MAX_PATH, fullpath, 0);

	std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
	texture->subresources.clear(); 
	texture->subresources.push_back(D3D12_SUBRESOURCE_DATA());

	D3D12_SUBRESOURCE_DATA& subresource = texture->subresources[0]; // Add this member

	//Load bmp using WICLoader
	DirectX::LoadWICTextureFromFile(device.GetDevice(), fullpath, &texture->texture, texture->decodedData, texture->subresources[0]);

	if (!texture->texture)
	{
		std::wcout << "Failed to load texture: " << std::wstring(fullpath) << std::endl;
		return nullptr;
	}

	//alloc descriptor
	heapAllocator.Alloc(&texture->cpuHandle, &texture->gpuHandle);

	device.AddRenderCommand([texture](D3D12GraphicsDevice& device)
		{
			//subresource data
			D3D12_SUBRESOURCE_DATA& subresource = texture->subresources[0];


			//create gpu upload buffer
			UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->texture.Get(), 0, 1);
			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			D3D12_RESOURCE_DESC bufferDesc = {};
			bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			bufferDesc.Width = uploadBufferSize;
			bufferDesc.Height = 1;
			bufferDesc.DepthOrArraySize = 1;
			bufferDesc.MipLevels = 1;
			bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
			bufferDesc.SampleDesc.Count = 1;
			bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			CHECKHR(device.GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, IID_PPV_ARGS(&texture->uploadBuffer)));

			//copy texture
			UpdateSubresources(device.GetCommandList(), texture->texture.Get(), texture->uploadBuffer.Get(), 0, 0, 1, &subresource);


			auto Desc = texture->texture->GetDesc();
			//create shader resource view
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = Desc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;

			device.GetDevice()->CreateShaderResourceView(texture->texture.Get(), &srvDesc, texture->cpuHandle);

			texture->CleanupCPUMemory();
		});


	//set name string after last '/' or '\'
	std::wstring fullpathStr = fullpath;
	size_t lastSlash = fullpathStr.find_last_of(L"/\\");
	if (lastSlash != std::wstring::npos)
	{
		texture->name = fullpathStr.substr(lastSlash + 1);
	}
	else
	{
		texture->name = fullpathStr;
	}

	auto Desc = texture->texture->GetDesc();
	//fill texture desc
	TextureDesc& desc = texture->desc;
	desc.Width = Desc.Width;
	desc.Height = Desc.Height;
	desc.MipLevels = Desc.MipLevels;
	desc.Format = GetFormatString(Desc.Format);

	texture->uncompressedByteSize = (UINT)GetRequiredIntermediateSize(texture->texture.Get(), 0, 1);

	//get file size using std::filesystem
	std::filesystem::path path = fullpath;
	texture->compressedByteSize = (UINT)std::filesystem::file_size(path);

	return texture;
}

std::shared_ptr<Texture2D> Texture2D::CreateFromMemory(D3D12GraphicsDevice& device, const TextureCreateParams& Params)
{
	std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();

	size_t size = Params.data.size() * sizeof(uint8_t);

	// Create texture from memory (like simple back texture)
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, Params.width, (UINT)Params.height, (UINT16)size, D3D12_RESOURCE_FLAG_NONE);
	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	device.GetDevice()->CreateCommittedResource(&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_COPY_DEST, 0,
		IID_PPV_ARGS(&texture->texture));

	// Create upload buffer
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	CD3DX12_HEAP_PROPERTIES heapPropsUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->texture.Get(), 0, 1);
	device.GetDevice()->CreateCommittedResource(&heapPropsUpload,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 0,
		IID_PPV_ARGS(&texture->uploadBuffer));

	// Copy data to upload buffer in render command
	D3D12_SUBRESOURCE_DATA subresource = {};
	//memcopy
	subresource.pData = Params.data.data();
	subresource.RowPitch = size;
	subresource.SlicePitch = size;

	texture->subresources.push_back(subresource);

	//alloc descriptor
	heapAllocator.Alloc(&texture->cpuHandle, &texture->gpuHandle);

	device.AddRenderCommand([texture](D3D12GraphicsDevice& device)
		{
			//subresource data
			D3D12_SUBRESOURCE_DATA& subresource = texture->subresources[0];
			//copy texture
			UpdateSubresources(device.GetCommandList(), texture->texture.Get(), texture->uploadBuffer.Get(), 0, 0, 1, &subresource);
			auto Desc = texture->texture->GetDesc();
			//create shader resource view
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = Desc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
			device.GetDevice()->CreateShaderResourceView(texture->texture.Get(), &srvDesc, texture->cpuHandle);
			texture->CleanupCPUMemory();
		});

	//fill texture desc
	auto Desc = texture->texture->GetDesc();
	texture->desc.Width = Desc.Width;
	texture->desc.Height = Desc.Height;
	texture->desc.MipLevels = Desc.MipLevels;
	texture->desc.Format = GetFormatString(Desc.Format);

	return texture;
}

std::shared_ptr<Texture2D> Texture2D::CreateFromDDS(D3D12GraphicsDevice& device, const wchar_t* filename)
{
	std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
	//filename to fullpath
	wchar_t fullpath[MAX_PATH];
	GetFullPathName(filename, MAX_PATH, fullpath, 0);
	//Load DDS using WICLoader
	DirectX::LoadDDSTextureFromFile(device.GetDevice(), fullpath, &texture->texture, texture->decodedData, texture->subresources);
	if (!texture->texture)
	{
		std::wcout << "Failed to load texture: " << std::wstring(fullpath) << std::endl;
		return nullptr;
	}
	//alloc descriptor
	heapAllocator.Alloc(&texture->cpuHandle, &texture->gpuHandle);
	device.AddRenderCommand([texture](D3D12GraphicsDevice& device)
		{
			//subresource data
			D3D12_SUBRESOURCE_DATA& subresource = texture->subresources[0];
			//create gpu upload buffer
			UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->texture.Get(), 0, 1);
			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			D3D12_RESOURCE_DESC bufferDesc = {};
			bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			bufferDesc.Width = uploadBufferSize;
			bufferDesc.Height = 1;
			bufferDesc.DepthOrArraySize = 1;
			bufferDesc.MipLevels = 1;
			bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
			bufferDesc.SampleDesc.Count = 1;
			bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			CHECKHR(device.GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, IID_PPV_ARGS(&texture->uploadBuffer)));
			//copy texture
			UpdateSubresources(device.GetCommandList(), texture->texture.Get(), texture->uploadBuffer.Get(), 0, 0, 1, &subresource);
			auto Desc = texture->texture->GetDesc();
			//create shader resource view
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = Desc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
			device.GetDevice()->CreateShaderResourceView(texture->texture.Get(), &srvDesc, texture->cpuHandle);
			texture->CleanupCPUMemory();
		});
	//set name string after last '/' or '\'
	std::wstring fullpathStr = fullpath;
	size_t lastSlash = fullpathStr.find_last_of(L"/\\");
	if (lastSlash != std::wstring::npos)
	{
		texture->name = fullpathStr.substr(lastSlash + 1);
	}
	else
	{
		texture->name = fullpathStr;
	}
	auto Desc = texture->texture->GetDesc();
	//fill texture desc
	TextureDesc& desc = texture->desc;
	desc.Width = Desc.Width;
	desc.Height = Desc.Height;
	desc.MipLevels = Desc.MipLevels;
	desc.Format = GetFormatString(Desc.Format);
	texture->uncompressedByteSize = (UINT)GetRequiredIntermediateSize(texture->texture.Get(), 0, 1);
	//get file size using std::filesystem
	std::filesystem::path path = fullpath;
	texture->compressedByteSize = (UINT)std::filesystem::file_size(path);
	return texture;

}

// Get common simple names from DXGI format (ex. RGBA16, RGB)
std::wstring GetFormatString(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT: return L"RGBA32";
	case DXGI_FORMAT_R16G16B16A16_FLOAT: return L"RGBA16";
	case DXGI_FORMAT_R16G16B16A16_UNORM: return L"RGBA16";
	case DXGI_FORMAT_R8G8B8A8_UNORM: return L"RGBA8";
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return L"RGBA8";
	case DXGI_FORMAT_B8G8R8A8_UNORM: return L"BGRA8";
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return L"BGRA8";
	case DXGI_FORMAT_R10G10B10A2_UNORM: return L"RGB10A2";
	case DXGI_FORMAT_R16G16_FLOAT: return L"RG16";
	case DXGI_FORMAT_R16G16_UNORM: return L"RG16";
	case DXGI_FORMAT_R8G8_UNORM: return L"RG8";
	case DXGI_FORMAT_R8_UNORM: return L"R8";
	case DXGI_FORMAT_R16_FLOAT: return L"R16";
	case DXGI_FORMAT_R16_UNORM: return L"R16";
	case DXGI_FORMAT_BC1_UNORM: return L"BC1";
	case DXGI_FORMAT_BC1_UNORM_SRGB: return L"BC1";
	case DXGI_FORMAT_BC2_UNORM: return L"BC2";
	case DXGI_FORMAT_BC2_UNORM_SRGB: return L"BC2";
	case DXGI_FORMAT_BC3_UNORM: return L"BC3";
	case DXGI_FORMAT_BC3_UNORM_SRGB: return L"BC3";
	case DXGI_FORMAT_BC4_UNORM: return L"BC4";
	case DXGI_FORMAT_BC4_SNORM: return L"BC4";
	case DXGI_FORMAT_BC5_UNORM: return L"BC5";
	case DXGI_FORMAT_BC5_SNORM: return L"BC5";
	case DXGI_FORMAT_BC6H_UF16: return L"BC6H";
	case DXGI_FORMAT_BC6H_SF16: return L"BC6H";
	case DXGI_FORMAT_BC7_UNORM: return L"BC7";
	case DXGI_FORMAT_BC7_UNORM_SRGB: return L"BC7";
	default: return L"Unknown";
	}
}

// declaration default textures
std::shared_ptr<Texture2D> Texture2D::WhiteTexture;
std::shared_ptr<Texture2D> Texture2D::BlackTexture;
