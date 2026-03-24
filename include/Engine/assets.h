#pragma once



struct Assets {
	Assets(VulkanApi* api, VkDescriptor* desc, VkCmdModule* cmd);
	Assets(Dx11Api* api);

	~Assets();

	void loadToVk();
	void loadToDX();

private:
	VulkanApi* vkApi;
	VkDescriptor* descriptor;
	VkCmdModule* cmdPool;

	Dx11Api* dxApi;

};