#include "Engine/textures.h"
#include "Engine/stbi_image.h"
#include <filesystem>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

namespace fs = std::filesystem;

std::string textureDir = RESOURCES_PATH "textures";
std::string cacheDirs = RESOURCES_PATH "cache/textures";

namespace tex {
	uint32_t id = 1;
	std::mutex mtx;
	uint64_t ringSize = 0;
};

struct UploadSlice {
	size_t offset;
	size_t size;
};

UploadSlice allocateUpload(VkBuff* buff, size_t size);
void createTexturesKtx(std::string path, std::vector<TexturesInfoKtx>& ktxTextures);

Textures::Textures(VulkanApi* ren, VkDescriptor* descriptor, VkCommandPool cmdPool)
	:ren(ren), descriptor(descriptor), cmdPool(cmdPool) {}

void Textures::load() {
	loadToKtx();
	ringBuff = new VkBuff(ren->getAllocator(),
		{
			.usage = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			.memory = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.size = tex::ringSize,
		}
	);

	ktxToImage();
	isComplete = true;
}

void Textures::loadToKtx() {
	std::vector<std::thread> threads;
	for (const auto& entry : fs::directory_iterator(textureDir)) {
		threads.emplace_back(createTexturesKtx, entry.path().string(), std::ref(ktxTextures));
	}

	for (auto& t : threads)
		t.join();
}



VkTexture* Textures::ktxToVkTex(ktxTexture1* ktx) {
	size_t imageSize = ktx->baseWidth * ktx->baseHeight * 4;
	UploadSlice slice = allocateUpload(ringBuff, imageSize);

	memcpy((char*)ringBuff->getAllocInfo().pMappedData + slice.offset, ktx->pData, imageSize);


	VkTexture* tx = new VkTexture({
			.device = ren->getDevice(),
			.physicalDevice = ren->getPhysicalDevice(),
			.allocator = ren->getAllocator(),
			.cmdPool = cmdPool,
			.graphicsQueue = ren->getGraphicsQueue(),
			.descriptor = descriptor,
			.ringBuff = ringBuff->getBuffer(),
			.offset = (uint32_t)slice.offset,
		},
		{
			.format = VK_FORMAT_R8G8B8A8_SRGB ,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.dimension = {ktx->baseWidth, ktx->baseHeight, 1},
			.data = ktx->pData,
		},
		{}
	);

	singleCmdBuffs.emplace_back(tx->getSingleCmdBuffer());

	ktxTexture_Destroy(ktxTexture(ktx));

	return tx;

}

void Textures::cleanUp() {
	endCmdBuffs((uint32_t)singleCmdBuffs.size(), singleCmdBuffs.data(), cmdPool, ren->getDevice(), ren->getGraphicsQueue());
	delete ringBuff;
}

UploadSlice allocateUpload(VkBuff* buff, size_t size) {
	size_t aligned = (size + 255) & ~255;

	UploadSlice slice{};
	slice.offset = buff->head;
	slice.size = aligned;

	buff->head += aligned;

	if (buff->head >= tex::ringSize)
		buff->head = 0;

	return slice;
}

void Textures::ktxToImage() {
	vkTextures.reserve(ktxTextures.size());
	for (auto& tex : ktxTextures) {
		TexturesInfo info{
			.id = tex::id,
			.name = tex.name,
			.diff = ktxToVkTex(tex.diff),
			.norm = ktxToVkTex(tex.norm),
			.disp = ktxToVkTex(tex.disp),
			.arm = ktxToVkTex(tex.arm),
		};
		vkTextures.emplace_back(info);
		tex::id++;
	}
}

std::string getFilename(std::string filename) {
	std::string name = filename;
	std::replace(name.begin(), name.end(), '\\', '/');
	name = name.substr(name.find_last_of('/') + 1);
	name = name.substr(0, name.find_last_of('.'));
	return name;
}


void createTexturesKtx(std::string path, std::vector<TexturesInfoKtx> &ktxTextures) {
	std::string name = getFilename(path);
	std::string cachePath;
	bool isPresent = false;
	for (const auto& entry : fs::directory_iterator(cacheDirs)) {
		isPresent = false;
		std::string dir = entry.path().string();
		if (dir == path) {
			path = dir;
			isPresent = true;
		}
	}

	if (!isPresent) {
		cachePath = cacheDirs + "/" + name;
		fs::create_directories(cachePath);
	}

	TexturesInfoKtx info = {
		.name = name,
	};

	for (const auto& entry : fs::directory_iterator(path)) {
		std::string path = entry.path().string();
		std::string texName = getFilename(path);

		ktxTexture1 *ktx = ImageLoader::fromfile(path);

		tex::mtx.lock();
		tex::ringSize += ktx->baseWidth * ktx->baseHeight * 4;
		tex::mtx.unlock();



		if (texName.find("diff") != std::string::npos)
			info.diff = ktx;
		else if (texName.find("nor") != std::string::npos)
			info.norm = ktx;
		else if (texName.find("disp") != std::string::npos)
			info.disp = ktx;
		else if (texName.find("arm") != std::string::npos)
			info.arm = ktx;
		else {
			std::cout << "type not found of: " << texName << std::endl;
			ktxTexture_Destroy(ktxTexture(ktx));
		}

		if (!isPresent) {
			std::string cacheFile = cachePath + "/" + texName + ".ktx";
			ktxTexture_WriteToNamedFile(ktxTexture(ktx), cacheFile.c_str());
		}
			
	};

	std::lock_guard<std::mutex> lock(tex::mtx);
	ktxTextures.emplace_back(info);
}

Textures::~Textures() {
	for (auto& tex : vkTextures) {
		delete tex.diff;
		delete tex.disp;
		delete tex.arm;
		delete tex.norm;
	}
}