#pragma once
#include "dx11_api.h"
#include <string>

struct DxImageSamplerInfo{
	D3D11_FILTER filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	D3D11_TEXTURE_ADDRESS_MODE addressU = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	D3D11_TEXTURE_ADDRESS_MODE addressV = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	D3D11_TEXTURE_ADDRESS_MODE addressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
};

struct DxImage {
	DxImage(ComPtr<ID3D11Device>& device, std::string& filename, const DxImageSamplerInfo& info);
	~DxImage();

	ComPtr<ID3D11ShaderResourceView>& getTexture() { return _texture; }
	ComPtr<ID3D11SamplerState>& getSampler() { return _sampler; }
private:
	ComPtr<ID3D11Device> device;

	ComPtr<ID3D11SamplerState> _sampler = nullptr;
	ComPtr<ID3D11ShaderResourceView> _texture = nullptr;
	DxImageSamplerInfo info;

	void CreateTextureView(std::string &filename);
	void CreateTextureViewFromDDS(std::string& filename);
	void CreateTextureSampler();

};