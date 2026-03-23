#include "Engine/renderer/directx/dx_image.h"
#include <iostream>

#include <DirectXTex.h>


ComPtr<ID3D11SamplerState> DxImage::CreateTextureSampler(ComPtr<ID3D11Device>& device, const DxImageSamplerInfo& info) {
	D3D11_SAMPLER_DESC samplDesc = {};
	samplDesc.Filter = info.filter;
	samplDesc.AddressU = info.addressU;
	samplDesc.AddressV = info.addressV;
	samplDesc.AddressW = info.addressW;

	ComPtr<ID3D11SamplerState> sampler;

	if (FAILED(device->CreateSamplerState(&samplDesc, &sampler))) {
		std::cerr << "DXD11: Failed to create texture sampler" << std::endl;
		return nullptr;
	}

	return sampler;
}

ComPtr<ID3D11ShaderResourceView> DxImage::CreateTextureView(ComPtr<ID3D11Device>& device, std::string& filename) {
	DirectX::TexMetadata metaData = {};
	DirectX::ScratchImage sratchImage = {};

	std::wstring str(filename.begin(), filename.end());
	

	if (FAILED(DirectX::LoadFromWICFile(str.c_str(), DirectX::WIC_FLAGS::WIC_FLAGS_FORCE_SRGB, &metaData, sratchImage))) {
		std::cout << "DXTEX: failed to load image" << std::endl;
		return nullptr;
	}

	ComPtr<ID3D11Resource> texture = nullptr;
	if (FAILED(DirectX::CreateTexture(
		device.Get(),
		sratchImage.GetImages(),
		sratchImage.GetImageCount(),
		metaData,
		&texture)))
	{
		std::cerr << "DXTEX: Failed to create texture out of image" << std::endl;
		sratchImage.Release();
		return nullptr;
	}

	ID3D11ShaderResourceView* srv = nullptr;

	if (FAILED(DirectX::CreateShaderResourceView(
		device.Get(),
		sratchImage.GetImages(),
		sratchImage.GetImageCount(),
		metaData,
		&srv
	))) {
		std::cerr << "DXTEX: Failed to create shader resources view out of image" << std::endl;
		sratchImage.Release();
		return nullptr;
	}

	return srv;
}

ComPtr<ID3D11ShaderResourceView> DxImage::CreateTextureViewFromDDS(ID3D11Device* device, std::string& filename) {
	DirectX::TexMetadata metaData = {};
	DirectX::ScratchImage sratchImage = {};

	std::wstring str(filename.begin(), filename.end());

	if (FAILED(DirectX::LoadFromDDSFile(str.c_str(), DirectX::DDS_FLAGS_NONE, &metaData, sratchImage))) {
		std::cerr << "DXTex: Failed to load image" << std::endl;
		return nullptr;
	}

	ComPtr<ID3D11Resource> texture = nullptr;
	if (FAILED(DirectX::CreateTexture(
		device,
		sratchImage.GetImages(),
		sratchImage.GetImageCount(),
		metaData,
		&texture)))
	{
		std::cerr << "DXTEX: Failed to create texture out of image" << std::endl;
		sratchImage.Release();
		return nullptr;
	}

	ID3D11ShaderResourceView* srv = nullptr;

	if (FAILED(DirectX::CreateShaderResourceView(
		device,
		sratchImage.GetImages(),
		sratchImage.GetImageCount(),
		metaData,
		&srv
	))) {
		std::cerr << "DXTEX: Failed to create shader resources view out of image" << std::endl;
		sratchImage.Release();
		return nullptr;
	}

	return srv;
}