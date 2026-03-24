#include "Engine/textures.h"
#include <iostream>
#include <cassert>
#include <string> 
#include <filesystem>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;

unsigned int bufferSize = 0;

void loadTextures(std::string& dir, std::map<std::string, std::map<std::string, ktxTexture1*>>& textures);
void loadTexturesDx(std::string& dir,ComPtr<ID3D11Device>& device, std::map<std::string, std::map<std::string, DxImage*>> &textures);


Textures::Textures(VulkanApi* api, VkDescriptor* desc, VkCmdModule* cmd)
	: vkApi(api),descriptor(desc), cmdPool(cmd) {
}

Textures::Textures(Dx11Api* api) :dxApi(api) {
}

void Textures::loadToVk() {
	assert(vkApi);
	
	std::vector<std::string> texDirs;

	for (const auto& entry : fs::directory_iterator(RESOURCES_PATH"textures"))
		texDirs.emplace_back(entry.path().string());

	std::map<std::string, std::map<std::string, ktxTexture1*>> textures;
	{
		std::vector<std::jthread> ts;
		ts.reserve(texDirs.size());
		for (auto& str : texDirs)
			ts.emplace_back(loadTextures, std::ref(str), std::ref(textures));
	}

	// prepare ring buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;

	VkDeviceSize size = bufferSize;

	uint32_t head = 0;
	VkBuff::createStagingBuffer(vkApi->getAllocator(), size, stagingBuffer, stagingAllocation, stagingAllocInfo);

	for(auto &texs: textures)
		for (auto tex : texs.second) {
			ktxTexture1* ktx = tex.second;

			uint32_t offset = head;
			size_t imageSize = ktx->baseWidth * ktx->baseHeight * 4;

			head += imageSize;

			if (head >= bufferSize)
				head = 0;

			memcpy((char*)stagingAllocInfo.pMappedData + offset, ktx->pData, imageSize);
		}

	// coneverting to Vulkan Textures
	head = 0;
	for (auto& texs : textures) {
		std::map<std::string, VkTexture*> vkTexs;
		for (auto tex : texs.second) {
			ktxTexture1* ktx = tex.second;

			uint32_t offset = head;
			size_t imageSize = ktx->baseWidth * ktx->baseHeight * 4;

			VkTexture* texture = new VkTexture(
				VkTextureAttachInfo{
					.device = vkApi->getDevice(),
					.physicalDevice = vkApi->getPhysicalDevice(),
					.alloc = vkApi->getAllocator(),
					.cmd = cmdPool,
					.descriptor = descriptor,
					.ringBuff = stagingBuffer,
					.offset = offset,
				},
				VkTextureInfo{
					.extent = {ktx->baseWidth, ktx->baseHeight},
					.size = imageSize,
					.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				},
				{});

			head += imageSize;

			if (head >= bufferSize)
				head = 0;

			vkTexs.insert({tex.first, texture });
		}
		vkTextures.insert({ texs.first, vkTexs });
	}
	vmaDestroyBuffer(vkApi->getAllocator(), stagingBuffer, stagingAllocation);
}



void Textures::loadToDX() {
	assert(dxApi);
	std::vector<std::string> texDirs;

	for (const auto& entry : fs::directory_iterator(RESOURCES_PATH"textures")) {
		std::string dir = entry.path().string();
		texDirs.emplace_back(dir);

	}

	for (auto& str : texDirs) {
		loadTexturesDx(str, dxApi->getDevice(), dxTextures);
	}
		

}

Textures::~Textures() {
	if(vkApi != nullptr)
		for (auto& texs : vkTextures)
			for (auto tex : texs.second) 
				delete tex.second;
	else if (dxApi != nullptr)
		for (auto& texs : dxTextures)
			for (auto tex : texs.second)
				delete tex.second;
}


void loadTextures(std::string& dir, std::map<std::string, std::map<std::string, ktxTexture1*>> &textures) {
	std::map<std::string, ktxTexture1*> newTextures;
	for (const auto& entry : fs::directory_iterator(dir)) {
		std::string path = entry.path().string();
		
		ktxTexture1* ktxTex = loadKtxFromFile(path);
		if (ktxTex == nullptr)
			continue;

		std::string name = getFilename(path);

		size_t kBufferSize = ktxTex->baseWidth * ktxTex->baseHeight * 4;

		bufferSize += kBufferSize;

		if (name.find("nor_gl") != std::string::npos)
			newTextures.insert({ "normal_gl", ktxTex });
		else if (name.find("nor_dx") != std::string::npos)
			newTextures.insert({ "normal_dx", ktxTex });
		else if (name.find("disp") != std::string::npos)
			newTextures.insert({ "displacement", ktxTex });
		else if (name.find("arm") != std::string::npos)
			newTextures.insert({ "arm", ktxTex });
		else if (name.find("diff") != std::string::npos)
			newTextures.insert({ "diffuse", ktxTex });
		else {
			std::cerr << name << " type is not existsed" << std::endl;
			ktxTexture_Destroy(ktxTexture(ktxTex));
		}

	}

	std::string name = getFilename(dir);
	textures.insert({name, newTextures});
}

void loadTexturesDx(std::string& dir, ComPtr<ID3D11Device>& device, std::map<std::string, std::map<std::string, DxImage*>>& textures) {
	std::map<std::string, DxImage*> newTextures;
	for (const auto& entry : fs::directory_iterator(dir)) {
		std::string path = entry.path().string();
		DxImage* image = new DxImage(device, path, {});
		std::string name = getFilename(path);

		if (name.find("nor_gl") != std::string::npos)
			newTextures.insert({ "normal_gl", image });
		else if (name.find("nor_dx") != std::string::npos)
			newTextures.insert({ "normal_dx", image });
		else if (name.find("disp") != std::string::npos)
			newTextures.insert({ "displacement", image });
		else if (name.find("arm") != std::string::npos)
			newTextures.insert({ "arm", image });
		else if (name.find("diff") != std::string::npos)
			newTextures.insert({ "diffuse", image });
		else {
			std::cerr << name << " type is not existsed" << std::endl;
			delete image;
		}
	}

	std::string name = getFilename(dir);
	textures.insert({ name, newTextures });
}
