// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: a debug build's answer to PS5_DevLoop's grab (github.com/griches/PS5_DevLoop, AGENTS.md:
// "Giving a title grab"), for any program that shows its frames through Vulkan: the launcher (ui/gfx.cpp),
// the DS's and the libretro cores' screens (melonds/screens.cpp), and the programs of their own, which
// take this file from port/ (Eden and EglantinePS5 through PS5ECAM_PORT, as port/eden/ingame.h; the retro
// runtimes through tools/retro/prepare.py, as ecam_bridge.h). Only a debug build (PLAYTENDOH_DEBUG)
// includes it.
//
// It sits where a frame is shown, so a grab is what the screen shows: the game with whatever was drawn
// over it (in-game menus, loading screens), from whichever renderer drew it. Two calls change:
//
//   info.imageUsage = devloop::SwapchainUsage(info.imageUsage, caps.supportedUsageFlags);  // vkCreateSwapchainKHR
//   grab.SetSwapchain(swapchain, images, format, extent, info.imageUsage);                 // after it
//   result = grab.QueuePresent(queue, present, vkQueuePresentKHR);                         // for vkQueuePresentKHR
//
// The contract: every few frames the program tries to remove devloop-shot in its own folder, which fails
// on any ordinary start. When that works, the next frame shown is copied out (waited for once: a debug
// build's grab is worth a frame), and written on a thread of its own as a PNG into devloop-shots there,
// under another name first, then moved in whole, so the payload never takes half a picture. Above
// 1920 wide it is halved (a 4K frame as 1920x1080): DevLoop scales it to 1280 anyway.
//
// It loads the Vulkan functions it calls through the vkGetDeviceProcAddr it is given, so it needs nothing
// of the program's own loader, and writes its PNGs itself (stored, uncompressed), so it needs no library.
// A program that shows its frames otherwise (OpenGL: Eden's) defines DEVLOOP_NO_VULKAN and calls Asked
// before each frame it shows and, when it says so, SaveRgba with the frame read back.

#pragma once

#ifndef DEVLOOP_NO_VULKAN
#include <vulkan/vulkan.h>
#endif

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#ifndef DEVLOOP_NO_VULKAN
// the device functions a grab calls, loaded by Grab::Init
#define DEVLOOP_FUNCTIONS(X)                                                                                           \
	X(vkCreateCommandPool) X(vkDestroyCommandPool) X(vkAllocateCommandBuffers) X(vkResetCommandBuffer)                 \
	X(vkBeginCommandBuffer) X(vkEndCommandBuffer) X(vkCmdPipelineBarrier) X(vkCmdCopyImageToBuffer)                     \
	X(vkCreateBuffer) X(vkDestroyBuffer) X(vkGetBufferMemoryRequirements) X(vkAllocateMemory) X(vkFreeMemory)           \
	X(vkBindBufferMemory) X(vkMapMemory) X(vkInvalidateMappedMemoryRanges) X(vkCreateSemaphore) X(vkDestroySemaphore)   \
	X(vkCreateFence) X(vkDestroyFence) X(vkWaitForFences) X(vkResetFences) X(vkQueueSubmit)

#endif

namespace devloop
{
#ifndef DEVLOOP_NO_VULKAN
	// The swapchain's usage, with the copy a grab needs where the surface allows it
	inline VkImageUsageFlags SwapchainUsage(VkImageUsageFlags usage, VkImageUsageFlags supported)
	{
		return (supported & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) ? usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT : usage;
	}
#endif

	namespace detail
	{
		inline uint32_t Crc(uint32_t crc, const uint8_t* data, size_t size)
		{
			static uint32_t table[256];
			static bool made = false;
			if (!made)
			{
				for (uint32_t n = 0; n < 256; n++)
				{
					uint32_t c = n;
					for (int k = 0; k < 8; k++)
						c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
					table[n] = c;
				}
				made = true;
			}
			crc = ~crc;
			for (size_t i = 0; i < size; i++)
				crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
			return ~crc;
		}

		inline void Put32(std::vector<uint8_t>& out, uint32_t value)
		{
			for (int shift = 24; shift >= 0; shift -= 8)
				out.push_back(uint8_t(value >> shift));
		}

		inline void Chunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data)
		{
			Put32(out, (uint32_t)data.size());
			const size_t start = out.size();
			out.insert(out.end(), type, type + 4);
			out.insert(out.end(), data.begin(), data.end());
			Put32(out, Crc(0, out.data() + start, out.size() - start));
		}
	}

	// rgb: width * height * 3 bytes, rows top to bottom. A PNG with stored (uncompressed) deflate blocks.
	inline bool WritePng(const std::string& path, const std::vector<uint8_t>& rgb, uint32_t width, uint32_t height)
	{
		std::vector<uint8_t> raw;
		raw.reserve(size_t(height) * (width * 3 + 1));
		for (uint32_t y = 0; y < height; y++)
		{
			raw.push_back(0); // filter: none
			raw.insert(raw.end(), rgb.begin() + size_t(y) * width * 3, rgb.begin() + size_t(y + 1) * width * 3);
		}
		std::vector<uint8_t> zlib{0x78, 0x01};
		uint32_t a = 1, b = 0;
		for (size_t at = 0; at < raw.size();)
		{
			const size_t size = std::min<size_t>(65535, raw.size() - at);
			zlib.push_back(at + size == raw.size() ? 1 : 0);
			zlib.push_back(uint8_t(size));
			zlib.push_back(uint8_t(size >> 8));
			zlib.push_back(uint8_t(~size));
			zlib.push_back(uint8_t(~size >> 8));
			zlib.insert(zlib.end(), raw.begin() + at, raw.begin() + at + size);
			for (size_t i = at; i < at + size; i++)
			{
				a = (a + raw[i]) % 65521;
				b = (b + a) % 65521;
			}
			at += size;
		}
		detail::Put32(zlib, (b << 16) | a);
		std::vector<uint8_t> header;
		detail::Put32(header, width);
		detail::Put32(header, height);
		header.insert(header.end(), {8, 2, 0, 0, 0}); // 8 bits, RGB
		std::vector<uint8_t> png{0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
		detail::Chunk(png, "IHDR", header);
		detail::Chunk(png, "IDAT", zlib);
		detail::Chunk(png, "IEND", {});
		FILE* file = std::fopen(path.c_str(), "wb");
		if (!file)
			return false;
		const bool written = std::fwrite(png.data(), 1, png.size(), file) == png.size();
		return std::fclose(file) == 0 && written;
	}

	using Log = std::function<void(const std::string&)>;

	// Whether the payload asked for a frame: devloop-shot removed from folder, tried every fourth call
	// (calls: the caller's own count). Any renderer's, before it shows a frame.
	inline bool Asked(const std::string& folder, uint64_t& calls)
	{
		return ++calls % 4 == 0 && unlink((folder + "/devloop-shot").c_str()) == 0;
	}

	namespace detail
	{
		// rgb written as devloop-shots/frame-N.png in folder, on a thread of its own: under another name
		// first, then moved in whole, so the payload never takes half a picture
		inline void WriteAsync(const std::string& folder, std::vector<uint8_t> rgb, uint32_t width, uint32_t height, const Log& log)
		{
			static std::atomic<int> taken{0};
			const std::string shots = folder + "/devloop-shots";
			const std::string writing = folder + "/devloop-shot-writing.png";
			const std::string picture = shots + "/frame-" + std::to_string(++taken) + ".png";
			std::thread([rgb = std::move(rgb), width, height, shots, writing, picture, log] {
				mkdir(shots.c_str(), 0777);
				chmod(shots.c_str(), 0777);
				bool done = WritePng(writing, rgb, width, height);
				done = done && chmod(writing.c_str(), 0666) == 0 && std::rename(writing.c_str(), picture.c_str()) == 0;
				if (log)
					log(done ? "[devloop] " + picture : "[devloop] " + picture + " could not be written: " + std::strerror(errno));
			}).detach();
		}

		// a frame as 8-bit RGB, rows top to bottom, halved above 1920 wide (pixel: a pixel's RGB from x, y)
		template <typename Pixel>
		std::vector<uint8_t> Rgb(uint32_t& width, uint32_t& height, Pixel pixel)
		{
			const uint32_t step = width > 1920 ? 2 : 1;
			const uint32_t outWidth = width / step, outHeight = height / step;
			std::vector<uint8_t> rgb(size_t(outWidth) * outHeight * 3);
			for (uint32_t y = 0; y < outHeight; y++)
				for (uint32_t x = 0; x < outWidth; x++)
				{
					uint32_t sum[3] = {};
					for (uint32_t dy = 0; dy < step; dy++)
						for (uint32_t dx = 0; dx < step; dx++)
						{
							uint32_t c[3];
							pixel(x * step + dx, y * step + dy, c);
							for (int i = 0; i < 3; i++)
								sum[i] += c[i];
						}
					for (int i = 0; i < 3; i++)
						rgb[(size_t(y) * outWidth + x) * 3 + i] = uint8_t(sum[i] / (step * step));
				}
			width = outWidth;
			height = outHeight;
			return rgb;
		}
	}

	// A frame read back as 8-bit RGBA (OpenGL's glReadPixels: bottomUp, rows from the bottom), written as
	// the payload's picture. For a renderer that shows its frames otherwise than through Vulkan.
	inline void SaveRgba(const std::string& folder, const uint8_t* rgba, uint32_t width, uint32_t height, bool bottomUp, const Log& log)
	{
		const uint32_t fullHeight = height;
		const uint32_t pitch = width;
		std::vector<uint8_t> rgb = detail::Rgb(width, height, [&](uint32_t x, uint32_t y, uint32_t c[3]) {
			const uint8_t* p = rgba + (size_t(bottomUp ? fullHeight - 1 - y : y) * pitch + x) * 4;
			c[0] = p[0], c[1] = p[1], c[2] = p[2];
		});
		detail::WriteAsync(folder, std::move(rgb), width, height, log);
	}

#ifndef DEVLOOP_NO_VULKAN
	class Grab
	{
	public:

		Grab() = default;
		Grab(const Grab&) = delete;
		Grab& operator=(const Grab&) = delete;
		~Grab() { Destroy(); }

		// folder: the program's own folder, where the payload puts devloop-shot and finds devloop-shots.
		// queueFamily: the family of the queue frames are presented on.
		bool Init(VkDevice device, PFN_vkGetDeviceProcAddr getProc, const VkPhysicalDeviceMemoryProperties& memory,
			uint32_t queueFamily, std::string folder, Log log = {})
		{
			Destroy();
			m_device = device;
			m_memory = memory;
			m_folder = std::move(folder);
			m_log = std::move(log);
			bool loaded = true;
#define DEVLOOP_LOAD(name) loaded &= (m_##name = reinterpret_cast<PFN_##name>(getProc(device, #name))) != nullptr;
			DEVLOOP_FUNCTIONS(DEVLOOP_LOAD)
#undef DEVLOOP_LOAD
			if (!loaded)
				return Fail("a Vulkan function it needs is missing");
			VkCommandPoolCreateInfo pool{};
			pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			pool.queueFamilyIndex = queueFamily;
			VkCommandBufferAllocateInfo allocate{};
			allocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocate.commandBufferCount = 1;
			VkSemaphoreCreateInfo semaphore{};
			semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkFenceCreateInfo fence{};
			fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			if (m_vkCreateCommandPool(device, &pool, nullptr, &m_pool) != VK_SUCCESS)
				return Fail("no command pool");
			allocate.commandPool = m_pool;
			if (m_vkAllocateCommandBuffers(device, &allocate, &m_commands) != VK_SUCCESS ||
				m_vkCreateSemaphore(device, &semaphore, nullptr, &m_copied) != VK_SUCCESS ||
				m_vkCreateFence(device, &fence, nullptr, &m_done) != VK_SUCCESS)
				return Fail("no command buffer, semaphore or fence");
			m_ready = true;
			Say("ready: " + m_folder + "/devloop-shot asks for a frame");
			return true;
		}

		// After each vkCreateSwapchainKHR: its images, and the usage it was made with (SwapchainUsage's)
		void SetSwapchain(VkSwapchainKHR swapchain, std::vector<VkImage> images, VkFormat format, VkExtent2D extent,
			VkImageUsageFlags usage)
		{
			m_swapchain = swapchain;
			m_images = std::move(images);
			m_format = format;
			m_extent = extent;
			m_copyable = (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
			if (!m_copyable)
				Say("this swapchain's images cannot be copied (the surface does not allow it): no grabs");
			else if (!Supported(format))
				Say("this swapchain's format (" + std::to_string((int)format) + ") is not one a grab reads: no grabs");
		}

		// In place of vkQueuePresentKHR (present: the program's own pointer to it)
		VkResult QueuePresent(VkQueue queue, const VkPresentInfoKHR& info, PFN_vkQueuePresentKHR present)
		{
			if (!Asked(info))
				return present(queue, &info);
			const uint32_t image = info.pImageIndices[0];
			if (!m_copyable || !Supported(m_format) || image >= m_images.size() || !Buffer())
				return present(queue, &info);

			m_vkResetCommandBuffer(m_commands, 0);
			VkCommandBufferBeginInfo begin{};
			begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			m_vkBeginCommandBuffer(m_commands, &begin);
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_images[image];
			barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			m_vkCmdPipelineBarrier(m_commands, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
				nullptr, 1, &barrier);
			VkBufferImageCopy region{};
			region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			region.imageExtent = {m_extent.width, m_extent.height, 1};
			m_vkCmdCopyImageToBuffer(m_commands, m_images[image], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_buffer, 1, &region);
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = 0;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			m_vkCmdPipelineBarrier(m_commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
				nullptr, 1, &barrier);
			m_vkEndCommandBuffer(m_commands);

			// the copy waits for what the present would have waited for, and the present for the copy
			const std::vector<VkPipelineStageFlags> stages(info.waitSemaphoreCount, VK_PIPELINE_STAGE_TRANSFER_BIT);
			VkSubmitInfo submit{};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.waitSemaphoreCount = info.waitSemaphoreCount;
			submit.pWaitSemaphores = info.pWaitSemaphores;
			submit.pWaitDstStageMask = stages.data();
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &m_commands;
			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = &m_copied;
			if (m_vkQueueSubmit(queue, 1, &submit, m_done) != VK_SUCCESS)
			{
				Say("the copy could not be submitted");
				return present(queue, &info);
			}
			VkPresentInfoKHR shown = info;
			shown.waitSemaphoreCount = 1;
			shown.pWaitSemaphores = &m_copied;
			const VkResult result = present(queue, &shown);

			m_vkWaitForFences(m_device, 1, &m_done, VK_TRUE, UINT64_MAX);
			m_vkResetFences(m_device, 1, &m_done);
			VkMappedMemoryRange range{};
			range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range.memory = m_bufferMemory;
			range.size = VK_WHOLE_SIZE;
			m_vkInvalidateMappedMemoryRanges(m_device, 1, &range);
			Save(static_cast<const uint8_t*>(m_mapped));
			return result;
		}

		// Before the device goes (with nothing of its own in flight)
		void Destroy()
		{
			if (!m_device)
				return;
			if (m_buffer)
				m_vkDestroyBuffer(m_device, m_buffer, nullptr);
			if (m_bufferMemory)
				m_vkFreeMemory(m_device, m_bufferMemory, nullptr);
			if (m_copied)
				m_vkDestroySemaphore(m_device, m_copied, nullptr);
			if (m_done)
				m_vkDestroyFence(m_device, m_done, nullptr);
			if (m_pool)
				m_vkDestroyCommandPool(m_device, m_pool, nullptr);
			m_buffer = VK_NULL_HANDLE;
			m_bufferMemory = VK_NULL_HANDLE;
			m_mapped = nullptr;
			m_bufferSize = 0;
			m_copied = VK_NULL_HANDLE;
			m_done = VK_NULL_HANDLE;
			m_pool = VK_NULL_HANDLE;
			m_commands = VK_NULL_HANDLE;
			m_swapchain = VK_NULL_HANDLE;
			m_images.clear();
			m_device = VK_NULL_HANDLE;
			m_ready = false;
		}

	private:
#define DEVLOOP_MEMBER(name) PFN_##name m_##name = nullptr;
		DEVLOOP_FUNCTIONS(DEVLOOP_MEMBER)
#undef DEVLOOP_MEMBER

		static bool Supported(VkFormat format)
		{
			switch (format)
			{
			case VK_FORMAT_B8G8R8A8_UNORM:
			case VK_FORMAT_B8G8R8A8_SRGB:
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_R8G8B8A8_SRGB:
			case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
				return true;
			default:
				return false;
			}
		}

		// this present, if the payload asked for it: the request file removed (every fourth present)
		bool Asked(const VkPresentInfoKHR& info)
		{
			if (!m_ready || info.swapchainCount != 1 || info.pSwapchains[0] != m_swapchain)
				return false;
			return devloop::Asked(m_folder, m_presents);
		}

		// the buffer the frame is copied into, made the first time and when the frame grows
		bool Buffer()
		{
			const VkDeviceSize size = VkDeviceSize(m_extent.width) * m_extent.height * 4;
			if (m_buffer && m_bufferSize >= size)
				return true;
			if (m_buffer)
				m_vkDestroyBuffer(m_device, m_buffer, nullptr);
			if (m_bufferMemory)
				m_vkFreeMemory(m_device, m_bufferMemory, nullptr);
			m_buffer = VK_NULL_HANDLE;
			m_bufferMemory = VK_NULL_HANDLE;
			m_mapped = nullptr;
			m_bufferSize = 0;
			VkBufferCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			info.size = size;
			info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			if (m_vkCreateBuffer(m_device, &info, nullptr, &m_buffer) != VK_SUCCESS)
				return Fail("no buffer for the frame");
			VkMemoryRequirements requirements;
			m_vkGetBufferMemoryRequirements(m_device, m_buffer, &requirements);
			uint32_t type = UINT32_MAX;
			for (uint32_t i = 0; i < m_memory.memoryTypeCount && type == UINT32_MAX; i++)
				if ((requirements.memoryTypeBits & (1u << i)) &&
					(m_memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
					type = i;
			VkMemoryAllocateInfo allocate{};
			allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocate.allocationSize = requirements.size;
			allocate.memoryTypeIndex = type;
			if (type == UINT32_MAX || m_vkAllocateMemory(m_device, &allocate, nullptr, &m_bufferMemory) != VK_SUCCESS ||
				m_vkBindBufferMemory(m_device, m_buffer, m_bufferMemory, 0) != VK_SUCCESS ||
				m_vkMapMemory(m_device, m_bufferMemory, 0, VK_WHOLE_SIZE, 0, &m_mapped) != VK_SUCCESS)
				return Fail("no memory the frame can be read from");
			m_bufferSize = size;
			return true;
		}

		// the copied frame as 8-bit RGB, written apart from the program
		void Save(const uint8_t* pixels)
		{
			const VkFormat format = m_format;
			const uint32_t pitch = m_extent.width;
			uint32_t width = m_extent.width, height = m_extent.height;
			std::vector<uint8_t> rgb = detail::Rgb(width, height, [&](uint32_t x, uint32_t y, uint32_t c[3]) {
				Pixel(format, pixels + (size_t(y) * pitch + x) * 4, c);
			});
			detail::WriteAsync(m_folder, std::move(rgb), width, height, m_log);
		}

		static void Pixel(VkFormat format, const uint8_t* p, uint32_t c[3])
		{
			uint32_t v;
			std::memcpy(&v, p, 4);
			switch (format)
			{
			case VK_FORMAT_B8G8R8A8_UNORM:
			case VK_FORMAT_B8G8R8A8_SRGB:
				c[0] = p[2], c[1] = p[1], c[2] = p[0];
				break;
			case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
				c[0] = (v & 1023) >> 2, c[1] = (v >> 10 & 1023) >> 2, c[2] = (v >> 20 & 1023) >> 2;
				break;
			case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
				c[0] = (v >> 20 & 1023) >> 2, c[1] = (v >> 10 & 1023) >> 2, c[2] = (v & 1023) >> 2;
				break;
			default: // R8G8B8A8
				c[0] = p[0], c[1] = p[1], c[2] = p[2];
				break;
			}
		}

		void Say(const std::string& line)
		{
			if (m_log)
				m_log("[devloop] " + line);
		}

		bool Fail(const std::string& why)
		{
			Say("grab off: " + why);
			return false;
		}

		VkDevice m_device = VK_NULL_HANDLE;
		VkPhysicalDeviceMemoryProperties m_memory{};
		std::string m_folder;
		Log m_log;
		bool m_ready = false;
		VkCommandPool m_pool = VK_NULL_HANDLE;
		VkCommandBuffer m_commands = VK_NULL_HANDLE;
		VkSemaphore m_copied = VK_NULL_HANDLE;
		VkFence m_done = VK_NULL_HANDLE;
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_bufferMemory = VK_NULL_HANDLE;
		void* m_mapped = nullptr;
		VkDeviceSize m_bufferSize = 0;
		VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
		std::vector<VkImage> m_images;
		VkFormat m_format = VK_FORMAT_UNDEFINED;
		VkExtent2D m_extent{};
		bool m_copyable = false;
		uint64_t m_presents = 0;
	};
#endif
}
#undef DEVLOOP_FUNCTIONS
