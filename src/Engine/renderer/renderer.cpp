#include "Engine/renderer/renderer.h"
#include <cassert>
#include <iostream>

#ifdef _DEBUG
constexpr bool enableDebugger = true;
#else
constexpr bool enableDebugger = false;
#endif


Renderer::Renderer(Settings &set) : settings(set) {
	initWindow();
	bool vkWorked = false;


	if (settings.api == Vulkan) {
		vkApi = new VulkanApi(window, settings, enableDebugger);
		if (!vkApi->init()) {
			settings.api = Directx11;
			vkApi = nullptr;
			std::cerr << "failed to initialize vulkan" << std::endl;
		}
		else {
			std::cout << "vulkan initialized successfully" << std::endl;

			buff = new VkCmdModule(vkApi->getDevice(), vkApi->getGraphicsQueue(), vkApi->getQueueFamilyIndices().graphcisFamily.value());
			buff->createCommandBuffers(commandBuffers, MAX_FRAMES_IN_FLIGHTS);

			uint32_t MAX_TEXTURES = 1000;
			std::vector<VkDescriptorPoolSize> poolSize {
					{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_TEXTURES * 2},
					{VK_DESCRIPTOR_TYPE_SAMPLER, MAX_TEXTURES * 2},
					{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_TEXTURES * 2},
			};

			std::vector<VkDescriptorBindingFlags> bindingFlags = {
				// binding 0
				// (2D texture)
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
					VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
				// binding 1
				// (sampler)
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
					VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
				// binding 2
				// (cubemap)
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
				VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT,
			};


			std::vector<VkDescriptorSetLayoutBinding> uboLayout({
				// 2d textures
				{
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = MAX_TEXTURES,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				},
				{
					.binding = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
					.descriptorCount = MAX_TEXTURES,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				},
				// cubemap
				{
					.binding = 2,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = MAX_TEXTURES,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				},
				});

			VkDescriptorInfo info = {
				.pool = poolSize,
				.bindingFlags = bindingFlags,
				.uboLayouts = uboLayout,
				.MAX_TEXTURES = MAX_TEXTURES,
			};

			descriptor = new VkDescriptor(vkApi->getDevice(),info);
		}
	}

	if (settings.api == Directx11) {
		dx11Api = new Dx11Api(window, settings);
		if (!dx11Api->Initialize()) {
			std::cerr << "failed to intialize dx 11" << std::endl;
			assert(false);
		}
		std::cout << "Directx 11 initialized successfully" << std::endl;
	}

}

void Renderer::drawTriangle(uint32_t frameIndex) {
	// dx

	if (settings.api == Directx11) {


		if (_triangleBuff == nullptr) {

			std::vector<D3D11_INPUT_ELEMENT_DESC> vertexInputLayoutInfo{
			{
				"POSITION",
				0,
				DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				0,
				offsetof(Vertex, pos),
				D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				0,
			},
			{
				"COLOR",
				0,
				DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				0,
				offsetof(Vertex, color),
				D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				0,
			},
			};

			_vertexShader = dx11Api->createVertexShader(SHADERS_PATH"triangle.hlsl", vertexInputLayoutInfo, inputLayout);
			_pixelShader = dx11Api->createPixelShader(SHADERS_PATH"triangle.hlsl");

			Vertex vertices[] = {
				{  { 0.0f, 0.5f, 0.0f }, { 0.25f, 0.39f, 0.19f }},
				{  { 0.5f, -0.5f, 0.0f }, { 0.44f, 0.75f, 0.35f }},
				{  { -0.5f, -0.5f, 0.0f }, { 0.38f, 0.55f, 0.20f }},
			};

			D3D11_BUFFER_DESC bufferInfo = {
				.ByteWidth = sizeof(vertices),
				.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
				.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			};

			D3D11_SUBRESOURCE_DATA  resourcesData = {
				.pSysMem = vertices,
			};

			_triangleBuff = dx11Api->createBuffer(bufferInfo, resourcesData);
		}

		ComPtr<ID3D11DeviceContext>& _deviceContext = dx11Api->getDeviceContext();
		ComPtr<IDXGISwapChain1>& _swapChain = dx11Api->getSwapChain();

		constexpr UINT vertexStride = sizeof(Vertex);
		constexpr UINT vertexOffset = 0;

		_deviceContext->IASetInputLayout(inputLayout.Get());
		_deviceContext->IASetVertexBuffers(0, 1, _triangleBuff.GetAddressOf(), &vertexStride, &vertexOffset);
		_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_deviceContext->VSSetShader(_vertexShader.Get(), nullptr, 0);

		_deviceContext->PSSetShader(_pixelShader.Get(), nullptr, 0);

		_deviceContext->Draw(3, 0);

		_swapChain->Present(1, 0);
	}

	if (settings.api == Vulkan) {

		if (vertBuf == nullptr) {

			Vertex vertices[] = {
				{  { 0.0f, 0.5f, 0.0f }, { 0.25f, 0.39f, 0.19f }},
				{  { 0.5f, -0.5f, 0.0f }, { 0.44f, 0.75f, 0.35f }},
				{  { -0.5f, -0.5f, 0.0f }, { 0.38f, 0.55f, 0.20f }},
			};

			VkBuffInfo info = {
					.size = 3 * sizeof(Vertex),
					.data = vertices,
					.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					.type = Storage_Type,
				};

			vertBuf = new VkBuff(vkApi->getAllocator(), buff, info);

			pipeline = new VkPipelineModule(vkApi->getDevice(), VkPipelineInfo{
					.input = {
							.vertexInput = {
								{.location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
								{.location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)},
								},
					.binding = {
							.stride = sizeof(Vertex),
						},
					},
					.colorFormat = vkApi->getSwapChainFormat(),
					.setLayout = descriptor->getSetLayout(),
				});

			pipeline->addShaderStage(SHADERS_PATH"spvs/main.spv", "VsMain", VK_SHADER_STAGE_VERTEX_BIT);
			pipeline->addShaderStage(SHADERS_PATH"spvs/main.spv", "PsMain", VK_SHADER_STAGE_FRAGMENT_BIT);

			pipeline->initGraphics();

		}

		VkCommandBuffer& cmdBuff = commandBuffers[frameIndex];


		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, 1, &descriptor->getSets()[frameIndex], 0, nullptr);
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(cmdBuff, 0, 1, &vertBuf->getBuffer(), offsets);

		buff->cmdDraw(cmdBuff, 3);

	}


}


void Renderer::blackScreen() {
	uint32_t frameIndex = 0;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		if (settings.api == Directx11) {
			dx11Api->Render();
			drawTriangle();
		}
		else if (settings.api == Vulkan) {

			vkApi->acquireSwapChainIndex(frameIndex);
			uint32_t imageIndex = vkApi->getImageIndex();

			VkCommandBuffer& cmdBuff = commandBuffers[frameIndex];

			VkImage& image = vkApi->getSwapChainImages()[imageIndex];
			VkImageView& imageView = vkApi->getSwapChainImageViews()[imageIndex];

			buff->beginCommandBuffer(cmdBuff);

			BeginRenderInfo beginInfo{
				.swapExtent = vkApi->getSwapChainExtent(),
				.color = { {
						.image = image,
						.imageView = imageView,
					} },
			};

			buff->cmdBeginRendering(cmdBuff, beginInfo);
			drawTriangle(frameIndex);
			buff->cmdEndRendering(cmdBuff, image);
			buff->endCommandBuffer(cmdBuff);

			vkApi->submit({ cmdBuff }, frameIndex);


			frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHTS;
		}
	}

	if (settings.api == Vulkan)
		vkApi->waitDeviceIdle();

}

void Renderer::initWindow() {
	if (!glfwInit())
		throw std::runtime_error("failed to init glfw");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(
		settings.width, settings.height, settings.appName.c_str(),
		settings.win_mode == Fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);

	if (window == nullptr)
		throw std::runtime_error("failed to create window");

	glfwSetFramebufferSizeCallback(window, HandleResize);

}

void Renderer::HandleResize(GLFWwindow* window, int width, int height) {
	Renderer* ren = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	ren->onResize(width, height);
}

void Renderer::onResize(int width, int height) {
	settings.width = (uint32_t)width;
	settings.height = (uint32_t)height;
}

Renderer::~Renderer() {
	if (vkApi != nullptr) {
		delete buff;

		delete vkApi;
	}
	if (dx11Api != nullptr)
		delete dx11Api;
}
