#include "Game/sandbox.h"
#include <thread>
#include <iostream>
#include <cassert>
#include <chrono>

Sandbox::Sandbox(Renderer* ren, GameSettings& setings): ren_(ren), settings(setings) {

	secondCmdPool = ren->vkApi->createCommandPool();

	descriptor = new VkDescriptor(ren_->vkApi->getDevice(), {
				.poolSize = {
						{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_TEXTURES * MAX_FRAMES_IN_FLIGHT},
						{VK_DESCRIPTOR_TYPE_SAMPLER, MAX_TEXTURES * MAX_FRAMES_IN_FLIGHT},
					},
				.bindingFlags = {
						// binding 0
						// (images)
						VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
						VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
						// binding 1
						// (sampler)
						VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
						VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
					},
				.uboLayouts = {
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
					},
		});

	textures = new Textures(ren->vkApi, descriptor, secondCmdPool);
}

bool isInProcess = false;

void Sandbox::run() {
	while (!glfwWindowShouldClose(ren_->window)) {
		glfwPollEvents();

		ren_->blackScreen();


		if (!isInProcess) {
			std::thread j2([&]() {
				textures->load();
				});
			j2.detach();
			isInProcess = true;
		}

		if (textures->isCompleted()) {
			textures->cleanUp();
		}
	}

	ren_->vkApi->waitDeviceIdle();
	
}

Sandbox::~Sandbox() {
	delete descriptor;
	delete textures;
}