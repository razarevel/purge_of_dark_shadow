#pragma once

#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <string>

#include "Engine/settings.h"

#include <vector>

struct GLFWwindow;

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

struct Dx11Api {
	Dx11Api(GLFWwindow* window, const Settings& settings);
	~Dx11Api();

	bool Initialize();
	void Render();

	ComPtr<ID3D11DeviceContext>& getDeviceContext() { return _deviceContext; }
	ComPtr<IDXGISwapChain1>& getSwapChain() { return _swapChain; }

	ComPtr<ID3D11Buffer> createBuffer(D3D11_BUFFER_DESC &bufferInfo, D3D11_SUBRESOURCE_DATA &resourcesData);

	ComPtr<ID3D11VertexShader> createVertexShader(const char* filename, std::vector<D3D11_INPUT_ELEMENT_DESC>& infos, ComPtr<ID3D11InputLayout> &layout);
	ComPtr<ID3D11PixelShader> createPixelShader(const char* filename);

	ComPtr<ID3D11Device> &getDevice() { return _device; }


private:
	bool createSwapChainResources();
	void destroySwapChainResources();

	Settings settings;
	GLFWwindow* window;

	ComPtr<ID3D11Device> _device = nullptr;
	ComPtr<ID3D11DeviceContext> _deviceContext = nullptr;
	ComPtr<IDXGIFactory2> _dxgiFactory = nullptr;
	ComPtr<IDXGISwapChain1> _swapChain = nullptr;
	ComPtr<ID3D11RenderTargetView> _renderTarget = nullptr;
	ComPtr<ID3D11Debug> _debug = nullptr;

	ComPtr<ID3DBlob> CompileShader(const std::string& filename, const std::string& entryPoint, const std::string& profile);
};