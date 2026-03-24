#include "Engine/renderer/directx/dx_image.h"
#include <iostream>

#include <DirectXTex.h>

DxImage::DxImage(ComPtr<ID3D11Device>& device, std::string& filename, const DxImageSamplerInfo& info)
 :device(device), info(info) {
	if (filename.find("dds") != std::string::npos)
		CreateTextureViewFromDDS(filename);
	else
		CreateTextureView(filename);

	CreateTextureSampler();
}

void DxImage::CreateTextureSampler() {
	D3D11_SAMPLER_DESC samplDesc = {};
	samplDesc.Filter = info.filter;
	samplDesc.AddressU = info.addressU;
	samplDesc.AddressV = info.addressV;
	samplDesc.AddressW = info.addressW;

	ComPtr<ID3D11SamplerState> sampler;

	if (FAILED(device->CreateSamplerState(&samplDesc, &_sampler))) {
		std::cerr << "DXD11: Failed to create texture sampler" << std::endl;
	}

}

void DxImage::CreateTextureView(std::string& filename) {
	DirectX::TexMetadata metaData = {};
	DirectX::ScratchImage sratchImage = {};

	std::wstring str(filename.begin(), filename.end());
	

	if (FAILED(DirectX::LoadFromWICFile(str.c_str(), DirectX::WIC_FLAGS::WIC_FLAGS_FORCE_SRGB, &metaData, sratchImage))) {
		std::cout << "DXTEX: failed to load image" << std::endl;
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
	}


	if (FAILED(DirectX::CreateShaderResourceView(
		device.Get(),
		sratchImage.GetImages(),
		sratchImage.GetImageCount(),
		metaData,
		&_texture
	))) {
		std::cerr << "DXTEX: Failed to create shader resources view out of image" << std::endl;
		sratchImage.Release();
	}

}

void DxImage::CreateTextureViewFromDDS(std::string& filename) {
	DirectX::TexMetadata metaData = {};
	DirectX::ScratchImage sratchImage = {};

	std::wstring str(filename.begin(), filename.end());

	if (FAILED(DirectX::LoadFromDDSFile(str.c_str(), DirectX::DDS_FLAGS_NONE, &metaData, sratchImage))) {
		std::cerr << "DXTex: Failed to load image" << std::endl;
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
	}


	if (FAILED(DirectX::CreateShaderResourceView(
		device.Get(),
		sratchImage.GetImages(),
		sratchImage.GetImageCount(),
		metaData,
		&_texture
	))) {
		std::cerr << "DXTEX: Failed to create shader resources view out of image" << std::endl;
		sratchImage.Release();
	}

}

DxImage::~DxImage() {
	_sampler.Reset();
	_texture.Reset();
}