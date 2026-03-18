#pragma once

#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl.h>

#include "Engine/settings.h"

struct GLFWwindow;


struct Dx11Api {
	Dx11Api(GLFWwindow* window, const Settings& settings);
	~Dx11Api();

	bool Initialize();
private:
	bool createSwapChainResources();
	void destroySwapChainResources();

	Settings settings;
	GLFWwindow* window;

	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D11Device> _device = nullptr;
	ComPtr<ID3D11DeviceContext> _deviceContext = nullptr;
	ComPtr<IDXGIFactory2> _dxgiFactory = nullptr;
	ComPtr<IDXGISwapChain1> _swapChain = nullptr;
	ComPtr<ID3D11RenderTargetView> _renderTarget = nullptr;
};