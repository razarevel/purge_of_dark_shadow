#include "Engine/renderer/directx/dx11_api.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>
#include <cassert>


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
	uint32_t deviceFlags = 0;
#if !defined(NDEBUG)
	deviceFlags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
#endif


	if (FAILED(D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		deviceFlags,
		&deviceFeatureLevel,
		1,
		D3D11_SDK_VERSION,
		&_device,
		nullptr,
		&_deviceContext))) {
		std::cout << "D3D11:: failed to create device" << std::endl;
		return false;
	}

#if !defined(NDEBUG)
	if (FAILED(_device.As(&_debug))) {
		std::cerr << "D3D11: failed to create debug" << std::endl;
		return false;
	}
#endif

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

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {
		.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
	};

	if (FAILED(_device->CreateRenderTargetView(
		backBuffer.Get(),
		&rtvDesc,
		&_renderTarget
	))) {
		std::cerr << "D3D11: Failed to create RTV from back buffer" << std::endl;
		return false;
	}

	return true;
}

void Dx11Api::Render() {
	D3D11_VIEWPORT viewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = static_cast<float>(settings.width),
		.Height = static_cast<float>(settings.height),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};

	const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.f };

	_deviceContext->ClearRenderTargetView(_renderTarget.Get(), clearColor);
	_deviceContext->RSSetViewports(1, &viewport);
	_deviceContext->OMSetRenderTargets(1, _renderTarget.GetAddressOf(), nullptr);

}

void Dx11Api::destroySwapChainResources() {
	_renderTarget.Reset();
}

ComPtr<ID3D11Buffer> Dx11Api::createBuffer(D3D11_BUFFER_DESC& bufferInfo, D3D11_SUBRESOURCE_DATA& resourcesData) {
	ComPtr<ID3D11Buffer> buff = nullptr;
	if (FAILED(_device->CreateBuffer(
		&bufferInfo,
		&resourcesData,
		&buff))) {
		std::cerr << "D3D11: Failed to create triangle vertex buffer" << std::endl;
		assert(false);
	}
	return buff;
}

ComPtr<ID3D11VertexShader> Dx11Api::createVertexShader(const char* filename, std::vector<D3D11_INPUT_ELEMENT_DESC> &infos, ComPtr<ID3D11InputLayout> &layout) {
	ComPtr<ID3DBlob> blob = CompileShader(filename, "VsMain", "vs_5_0");

	ComPtr<ID3D11VertexShader> vertexShader;
	if (FAILED(_device->CreateVertexShader(
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		nullptr,
		&vertexShader))) {
		std::cerr << "D3D11: failed to create vertex shader" << std::endl;
		assert(false);
	}


	if (!infos.empty()) {
		if (FAILED(_device->CreateInputLayout(
			infos.data(),
			(UINT)infos.size(),
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			&layout)))
		{
			std::cerr << "D3D11: Failed to create default vertex input layout\n";
			return nullptr;
		}
	}

	return vertexShader;
}

ComPtr<ID3D11PixelShader> Dx11Api::createPixelShader(const char* filename) {
	ComPtr<ID3DBlob> blob = CompileShader(filename, "PsMain", "ps_5_0");

	ComPtr<ID3D11PixelShader> pixelShader;
	if (FAILED(_device->CreatePixelShader(
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		nullptr,
		&pixelShader))) {
		std::cerr << "D3D11: failed to create vertex shader" << std::endl;
		assert(false);
	}
	return pixelShader;
}

ComPtr<ID3DBlob> Dx11Api::CompileShader(const std::string& filename, const std::string& entryPoint, const std::string& profile) {
	const uint32_t compilerFlag = D3D10_SHADER_ENABLE_STRICTNESS;

	ComPtr<ID3DBlob> tempShaderBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	const std::wstring path(filename.begin(), filename.end());

	if (FAILED(D3DCompileFromFile(
		path.data(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint.data(),
		profile.data(),
		compilerFlag,
		0,
		&tempShaderBlob,
		&errorBlob
	))) {
		std::cerr << "D3D11::Failed to create read shader from file" << std::endl;
		std::cout << "Path:" << filename << std::endl;
		if (errorBlob != nullptr)
			std::cerr << "D3D11: With message: " << static_cast<const char*>(errorBlob->GetBufferPointer()) << std::endl;

		return nullptr;
	}
	
	return tempShaderBlob;
}


Dx11Api::~Dx11Api() {
	_deviceContext->Flush();
	destroySwapChainResources();
	_swapChain.Reset();
	_dxgiFactory.Reset();
	_deviceContext.Reset();

#if !defined(NDEBUG)
	_debug->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
#endif

	_device.Reset();
}