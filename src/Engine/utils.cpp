#include "Engine/utils.h"
#include <iostream>
#include <algorithm>
#include <cassert>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

std::string getFilename(std::string& filename) {
	std::string name = filename;
	std::replace(name.begin(), name.end(), '\\', '/');
	name = name.substr(name.find_last_of('/') + 1);
	name = name.substr(0, name.find_last_of('.'));
	return name;
}

ktxTexture1* loadKtxFromFile(std::string& filename) {
	int w, h;
	const stbi_uc* pixels = stbi_load(filename.c_str(), &w, &h, nullptr, 4);
	if (!pixels) {
		std::cout << "failed to load texture at: " << filename << std::endl;
		return nullptr;
	}

	ktxTexture1* ktxTex = nullptr;

	ktxTextureCreateInfo createInfo = {
	  .glInternalformat = GL_RGBA8,
	  .vkFormat = VK_FORMAT_R8G8B8A8_SRGB,
	  .baseWidth = (uint32_t)w,
	  .baseHeight = (uint32_t)h,
	  .baseDepth = 1,
	  .numDimensions = 2,
	  .numLevels = 1,
	  .numLayers = 1,
	  .numFaces = 1,
	  .generateMipmaps = VK_FALSE,
	};

	if (ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktxTex) != KTX_SUCCESS)
		assert(false);

	size_t kBufferSize = w * h * 4;
	memcpy(ktxTex->pData, (void*)pixels, kBufferSize);
	stbi_image_free((void*)pixels);

	return ktxTex;
}

