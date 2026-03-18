#include "Engine/renderer/directx/dx11_api.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

Dx11Api::Dx11Api(GLFWwindow* window, const Settings& settings):window(window), settings(settings) {}

bool Dx11Api::Initialize() {
	if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(&_dxgiFactory)))) {
		std::cerr << "DXGI: Failed to create factory" << std::endl;
		return false;
	}

	constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_1;

	if (FAILED(D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		&deviceFeatureLevel,
		1,
		D3D11_SDK_VERSION,
		&_device,
		nullptr,
		&_deviceContext))) {
		std::cout << "D3D11:: failed to create device" << std::endl;
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDes = {
		.Width = (UINT)(settings.width),
		.Height = (UINT)(settings.height),
		.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
		.SampleDesc = {
			.Count = 1,
			.Quality = 0,
		 },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 2,
		.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags = {},
	};
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor;
	if (settings.win_mode == Window)
		swapChainFullscreenDescriptor.Windowed = true;

	if (FAILED(_dxgiFactory->CreateSwapChainForHwnd(
			_device.Get(),
			glfwGetWin32Window(window),
			&swapChainDes,
			&swapChainFullscreenDescriptor,
			nullptr,
			&_swapChain
	))) {
		std::cerr << "D3D11: failed to create swap chain" << std::endl;
		return false;
	}

	if (!createSwapChainResources())
		return false;

	return true;
}

bool Dx11Api::createSwapChainResources() {
	ComPtr<ID3D11Texture2D> backBuffer = nullptr;
	if (FAILED(_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
		std::cerr << "D3D11: Failed to get back Buffer from the SwapChain" << std::endl;
		return false;
	}

	if (FAILED(_device->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		&_renderTarget
	))) {
		std::cerr << "D3D11: Failed to create RTV from back buffer" << std::endl;
		return false;
	}

	return true;
}

void Dx11Api::destroySwapChainResources() {
	_renderTarget.Reset();
}

Dx11Api::~Dx11Api() {
	_deviceContext->Flush();
	destroySwapChainResources();
	_swapChain.Reset();
	_dxgiFactory.Reset();
	_deviceContext.Reset();
	_device.Reset();
}