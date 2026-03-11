#pragma once 
#include <string>
#include "Engine/image_loader.h"
#include "Engine/renderer/renderer.h"
#include "Engine/renderer/vulkan/vk_texture.h"
#include "Engine/renderer/vulkan/vk_descriptor.h"
#include "Engine/renderer/vulkan/vk_buffer.h"
#include <vector>
#include "ktx.h"


struct TexturesInfoKtx {
	std::string name;
	ktxTexture1* diff = nullptr;
	ktxTexture1* norm = nullptr;
	ktxTexture1* disp = nullptr;
	ktxTexture1* arm = nullptr;
};

struct TexturesInfo {
	uint32_t id;
	std::string name;
	VkTexture* diff = nullptr;
	VkTexture* norm = nullptr;
	VkTexture* disp = nullptr;
	VkTexture* arm = nullptr;
};


struct Textures {
	Textures(VulkanApi *ren, VkDescriptor *descriptor, VkCommandPool cmdPool = VK_NULL_HANDLE);
	~Textures();

	bool isCompleted() { return isComplete; }
	void load();

	std::vector<VkCommandBuffer>& getCmdBuffsToSubmit() { return singleCmdBuffs; }
	VkBuff* getBuff() { return ringBuff; }

	void cleanUp();

private:
	VulkanApi* ren;
	VkDescriptor* descriptor;
	VkBuff* ringBuff;
	VkCommandPool cmdPool;
	std::vector<TexturesInfoKtx> ktxTextures;
	std::vector<TexturesInfo> vkTextures;
	bool isComplete = false;

	void loadToKtx();
	void ktxToImage();

	std::vector<VkCommandBuffer> singleCmdBuffs;


	VkTexture* ktxToVkTex(ktxTexture1* ktx);

};