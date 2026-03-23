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
	static ComPtr<ID3D11SamplerState> CreateTextureSampler(ComPtr<ID3D11Device>& device, const DxImageSamplerInfo& info);
	static ComPtr<ID3D11ShaderResourceView> CreateTextureView(ComPtr<ID3D11Device>& device, std::string& filename);
	static ComPtr<ID3D11ShaderResourceView> CreateTextureViewFromDDS(ID3D11Device *device, std::string &filename);
};