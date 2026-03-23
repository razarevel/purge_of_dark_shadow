#include "Engine/imgui_renderer.h"
#include <iostream>
#include <cassert>

struct ImGuiRendererImpl {
	std::vector<VkTexture*> textures_;
};

ImguiRenderer::ImguiRenderer(VulkanApi* api, GLFWwindow* window, VkDescriptor* desc, VkCmdModule* cmd)
	: vkApi(api), descriptor(desc), pimpl_(new ImGuiRendererImpl), cmdPool(cmd) {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "imgui-vulkan";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui_ImplGlfw_InitForOther(window, true);
}

void ImguiRenderer::createPipeline() {
	uint32_t nonLinearColorSpace = 1u;
	pipeline = new VkPipelineModule(vkApi->getDevice(), VkPipelineInfo{
		.specInfo = {
					.entries = {{.constantID = 0, .offset = 0, .size = sizeof(uint32_t)}},
					.dataSize = sizeof(nonLinearColorSpace),
					.data = &nonLinearColorSpace,
				},
		.enableBlend = true,
		.colorFormat = vkApi->getSwapChainFormat(),
		.setLayout = descriptor->getSetLayout(),
		});

	pipeline->addShaderStage(SHADERS_PATH"spvs/imgui.vspv", "main", VK_SHADER_STAGE_VERTEX_BIT);
	pipeline->addShaderStage(SHADERS_PATH"spvs/imgui.fspv", "main", VK_SHADER_STAGE_FRAGMENT_BIT, true);

	pipeline->initGraphics();
}

void ImguiRenderer::vkBeginFrame(const VkExtent2D& dim) {
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(dim.width / displaceScale, dim.height / displaceScale);
	io.DisplayFramebufferScale = ImVec2(displaceScale, displaceScale);
	io.IniFilename = nullptr;

	if (!pipeline)
		createPipeline();

	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImguiRenderer::vkEndFrame(VkCommandBuffer &cmdBuf, uint32_t fIndex) {
	ImGui::EndFrame();
	ImGui::Render();

	ImDrawData* dd = ImGui::GetDrawData();

	const float fb_width = dd->DisplaySize.x * dd->FramebufferScale.x;
	const float fb_height = dd->DisplaySize.y * dd->FramebufferScale.y;
	if (fb_width <= 0 || fb_height <= 0 || dd->CmdListsCount == 0)
		return;

	if(dd->Textures)
		for (ImTextureData* tex : *dd->Textures)
			switch (tex->Status) {
			case ImTextureStatus_OK:
				continue;
			case ImTextureStatus_Destroyed: 
				continue;
			case ImTextureStatus_WantCreate: {
				VkTexture* textuer = new VkTexture(
					VkTextureAttachInfo{
						.device = vkApi->getDevice(),
						.physicalDevice = vkApi->getPhysicalDevice(),
						.alloc = vkApi->getAllocator(),
						.cmd = cmdPool,
						.descriptor = descriptor,
					},
					VkTextureInfo{
						.extent = {(uint32_t)tex->Width, (uint32_t)tex->Height},
						.size = (uint32_t)tex->Width * tex->Height * 4,
						.data = tex->Pixels,
						.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
					},
					{});
				tex->SetTexID(textuer->getIndex());
				pimpl_->textures_.emplace_back(textuer);
				tex->SetStatus(ImTextureStatus_OK);
			}
				continue;
			case ImTextureStatus_WantUpdates: {
				VkTexture* texture = nullptr;
				for (auto* it : pimpl_->textures_)
					if (it->getIndex() == tex->GetTexID()) {
						texture = it;
						break;
					}

				texture->update(
					VkRect2D{
						.offset = {tex->UpdateRect.x, tex->UpdateRect.y},
						.extent = {tex->UpdateRect.w, tex->UpdateRect.h},
					}, 
					tex->GetPixelsAt(tex->UpdateRect.x, tex->UpdateRect.y),
					tex->Width);
				tex->SetStatus(ImTextureStatus_OK);
			}
				continue;
			case ImTextureStatus_WantDestroy:
				std::cout << "want to be desctroyed" << std::endl;
				assert(false);
			}

	cmdPool->cmdBindDepthState(cmdBuf);

	VkViewport viewport = {
		.x = 0.0f,
		.y = fb_height,
		.width = fb_width,
		.height = -fb_height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	const float L = dd->DisplayPos.x;
	const float R = dd->DisplayPos.x + dd->DisplaySize.x;
	const float T = dd->DisplayPos.y;
	const float B = dd->DisplayPos.y + dd->DisplaySize.y;

	const ImVec2 clip_off = dd->DisplayPos;
	const ImVec2 clip_scale = dd->FramebufferScale;

	DrawableData& drawableData = drawables_[frameIndex];
	frameIndex = (frameIndex + 1) % 2;

	if (drawableData.numAllocateIndices < dd->TotalIdxCount) {
		if (drawableData.ib_)
			delete drawableData.ib_;

		drawableData.ib_ = new VkBuff(vkApi->getAllocator(), cmdPool,
			VkBuffInfo{
				.size = dd->TotalIdxCount * sizeof(ImDrawIdx),
				.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				.type = Host_Visible,
			});
		drawableData.numAllocateIndices = dd->TotalIdxCount;
	}
	if (drawableData.numAllocateVertices < dd->TotalVtxCount) {
		if (drawableData.vb_)
			delete drawableData.vb_;

		drawableData.vb_ = new VkBuff(vkApi->getAllocator(), cmdPool,
			VkBuffInfo{
				.size = dd->TotalVtxCount * sizeof(ImDrawVert),
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				.type = Host_Visible,
			});
		drawableData.numAllocateVertices = dd->TotalVtxCount;

	}

	ImDrawVert* vert = (ImDrawVert*)drawableData.vb_->getAllocInfo().pMappedData;
	uint16_t* idx = (uint16_t*)drawableData.ib_->getAllocInfo().pMappedData;

	for (const ImDrawList* cmdList : dd->CmdLists) {
		memcpy(vert, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
		vert += cmdList->VtxBuffer.Size;
		idx += cmdList->IdxBuffer.Size;
	}

	vmaFlushAllocation(vkApi->getAllocator(), drawableData.vb_->getAllocation(), 0, dd->TotalVtxCount * sizeof(ImDrawVert));
	vmaFlushAllocation(vkApi->getAllocator(), drawableData.ib_->getAllocation(), 0, dd->TotalIdxCount * sizeof(ImDrawIdx));

	uint32_t idxoffset = 0;
	uint32_t vtxoffset = 0;


	vkCmdBindIndexBuffer(cmdBuf, drawableData.ib_->getBuffer(), 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, 1, &descriptor->getSets()[fIndex], 0, nullptr);

	for (const ImDrawList* cmdList : dd->CmdLists) {
		for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++) {
			const ImDrawCmd cmd = cmdList->CmdBuffer[cmd_i];
			ImVec2 clipMin((cmd.ClipRect.x - clip_off.x) * clip_scale.x,
				(cmd.ClipRect.y - clip_off.y) * clip_scale.y);
			ImVec2 clipMax((cmd.ClipRect.z - clip_off.x) * clip_scale.x,
				(cmd.ClipRect.w - clip_off.y) * clip_scale.y);

			// clang-format off
			if (clipMin.x < 0.0f) clipMin.x = 0.0f;
			if (clipMin.y < 0.0f) clipMin.y = 0.0f;
			if (clipMax.x > fb_width) clipMax.x = fb_width;
			if (clipMax.y > fb_height) clipMax.y = fb_height;
			if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
				continue;
			// clang-format on
			struct VulkanImguiBindData {
				float LRTB[4];
				uint64_t vb = 0;
				uint64_t textureId = 0;
				uint64_t samplerId = 0;
			} bindData{
				.LRTB = {L, R, T, B},
				.vb = vkApi->gpuAddress(drawableData.vb_->getBuffer()),
				.textureId = cmd.GetTexID(),
				.samplerId = 0,
			};

			vkCmdPushConstants(cmdBuf, pipeline->getLayout(), VK_SHADER_STAGE_ALL, 0, 256, &bindData);

			VkRect2D rect = {
						int32_t(clipMin.x), int32_t(clipMin.y),
					    uint32_t(clipMax.x - clipMin.x),
					    uint32_t(clipMax.y - clipMin.y)
			};
			vkCmdSetScissor(cmdBuf, 0, 1, &rect);

			cmdPool->cmdDrawIndex(cmdBuf, cmd.ElemCount, 1u, idxoffset + cmd.IdxOffset, int32_t(vtxoffset + cmd.VtxOffset));
		}
		idxoffset += cmdList->IdxBuffer.Size;
		vtxoffset += cmdList->VtxBuffer.Size;
	}

}


ImguiRenderer::ImguiRenderer(Dx11Api* api, GLFWwindow* window): dx11Api(api) {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui_ImplGlfw_InitForOther(window, true);
	ImGui_ImplDX11_Init(dx11Api->getDevice().Get(), dx11Api->getDeviceContext().Get());
}

void ImguiRenderer::dxBeginFrame() {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImguiRenderer::dxEndFrame() {
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

ImguiRenderer::~ImguiRenderer() {
	if (vkApi != nullptr) {
		for (uint32_t i = 0; i < 2; i++) {
			delete drawables_[i].vb_;
			delete drawables_[i].ib_;
		}

		for (auto& it : pimpl_->textures_)
			delete it;

		delete pipeline;

		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	else if (dx11Api != nullptr) {
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}
