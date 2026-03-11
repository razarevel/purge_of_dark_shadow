#include "Engine/image_loader.h"
#include "Engine/stbi_image.h"
#include <cassert>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

ktxTexture1 *ImageLoader::fromfile(std::string filename) {
	ktxTexture1* ktxTex = nullptr;

	if (filename.find(".ktx") != std::string::npos) {
		ktxTexture1_CreateFromNamedFile(
			filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);
	}
	else {
		int w, h;
		const stbi_uc* pxiels = stbi_load(filename.c_str(), &w, &h, nullptr, 4);
		if (!pxiels) {
			std::cout << "failed to load texture at: " << filename << std::endl;
			return nullptr;
		}


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
			  .generateMipmaps = false,
		};
		if (ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE,
			&ktxTex) != KTX_SUCCESS)
			assert(false);

		size_t kBufferSize = w * h * 4;
		memcpy(ktxTex->pData, (void*)pxiels, kBufferSize);
		stbi_image_free((void*)pxiels);
	}

	return ktxTex;
}