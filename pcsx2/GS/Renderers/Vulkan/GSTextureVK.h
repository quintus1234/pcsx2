/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021 PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "GS/GS.h"
#include "GS/Renderers/Common/GSTexture.h"
#include "common/Vulkan/Context.h"
#include "common/Vulkan/Texture.h"

class GSTextureVK final : public GSTexture
{
public:
	GSTextureVK(Type type, Format format, Vulkan::Texture texture);
	~GSTextureVK() override;

	static std::unique_ptr<GSTextureVK> Create(Type type, u32 width, u32 height, u32 levels, Format format, VkFormat vk_format);

	__fi Vulkan::Texture& GetTexture() { return m_texture; }
	__fi Vulkan::Texture* GetTexturePtr() { return &m_texture; }
	__fi VkFormat GetNativeFormat() const { return m_texture.GetFormat(); }
	__fi VkImage GetImage() const { return m_texture.GetImage(); }
	__fi VkImageView GetView() const { return m_texture.GetView(); }
	__fi VkImageLayout GetLayout() const { return m_texture.GetLayout(); }

	void* GetNativeHandle() const override;

	bool Update(const GSVector4i& r, const void* data, int pitch, int layer = 0) override;
	bool Map(GSMap& m, const GSVector4i* r = NULL, int layer = 0) override;
	void Unmap() override;
	void GenerateMipmap() override;
	void Swap(GSTexture* tex) override;

	void TransitionToLayout(VkImageLayout layout);
	void CommitClear();
	void CommitClear(VkCommandBuffer cmdbuf);

	/// Framebuffers are lazily allocated.
	VkFramebuffer GetFramebuffer(bool feedback_loop);

	VkFramebuffer GetLinkedFramebuffer(GSTextureVK* depth_texture, bool feedback_loop);

	// Call when the texture is bound to the pipeline, or read from in a copy.
	__fi void SetUsedThisCommandBuffer()
	{
		m_use_fence_counter = g_vulkan_context->GetCurrentFenceCounter();
	}

private:
	VkCommandBuffer GetCommandBufferForUpdate();
	void CopyTextureDataForUpload(void* dst, const void* src, u32 pitch, u32 upload_pitch, u32 height) const;
	VkBuffer AllocateUploadStagingBuffer(const void* data, u32 pitch, u32 upload_pitch, u32 height) const;

	Vulkan::Texture m_texture;

	// Contains the fence counter when the texture was last used.
	// When this matches the current fence counter, the texture was used this command buffer.
	u64 m_use_fence_counter = 0;

	GSVector4i m_map_area = GSVector4i::zero();
	u32 m_map_level = UINT32_MAX;

	// linked framebuffer is combined with depth texture
	// list of color textures this depth texture is linked to or vice versa
	std::vector<std::tuple<GSTextureVK*, VkFramebuffer, bool>> m_framebuffers;
};

class GSDownloadTextureVK final : public GSDownloadTexture
{
public:
	~GSDownloadTextureVK() override;

	static std::unique_ptr<GSDownloadTextureVK> Create(u32 width, u32 height, GSTexture::Format format);

	void CopyFromTexture(const GSVector4i& drc, GSTexture* stex, const GSVector4i& src, u32 src_level, bool use_transfer_pitch) override;

	bool Map(const GSVector4i& read_rc) override;
	void Unmap() override;

	void Flush() override;

private:
	GSDownloadTextureVK(u32 width, u32 height, GSTexture::Format format);

	VmaAllocation m_allocation = VK_NULL_HANDLE;
	VkBuffer m_buffer = VK_NULL_HANDLE;

	u64 m_copy_fence_counter = 0;
	u32 m_buffer_size = 0;

	bool m_needs_cache_invalidate = false;
};
