// SPDX-License-Identifier: GPL-3.0-or-later
#include "DevLoopPS5.h"

#ifdef PLAYTENDOH_DEBUG
#include "../app/paths.h"
#include "../devloop/grab.h"

#include <cstdio>
#include <string>

namespace
{
	devloop::Grab s_grab;
	VkDevice s_device = VK_NULL_HANDLE; // the device the grab was made for
	VkSwapchainKHR s_swapchain = VK_NULL_HANDLE;
	bool s_ready = false;
}
#endif

VkResult PS5Cemu_Present(VkQueue queue, const VkPresentInfoKHR& info, bool tv, VkDevice device, VkPhysicalDevice physical,
	uint32_t queueFamily, const std::vector<VkImage>& images, VkFormat format, VkExtent2D extent)
{
#ifdef PLAYTENDOH_DEBUG
	if (tv && info.swapchainCount == 1)
	{
		if (s_device != device)
		{
			s_device = device;
			s_swapchain = VK_NULL_HANDLE;
			VkPhysicalDeviceMemoryProperties memory{};
			vkGetPhysicalDeviceMemoryProperties(physical, &memory);
			s_ready = s_grab.Init(device, vkGetDeviceProcAddr, memory, queueFamily, ps5paths::AppDir(),
				[](const std::string& line) { std::printf("[cemu] %s\n", line.c_str()); });
		}
		if (s_ready)
		{
			// made again when Cemu makes its swapchain again (a resize, the output's refresh rate)
			if (s_swapchain != info.pSwapchains[0])
			{
				s_swapchain = info.pSwapchains[0];
				s_grab.SetSwapchain(s_swapchain, images, format, extent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			}
			return s_grab.QueuePresent(queue, info, vkQueuePresentKHR);
		}
	}
#else
	(void)tv, (void)device, (void)physical, (void)queueFamily, (void)images, (void)format, (void)extent;
#endif
	return vkQueuePresentKHR(queue, &info);
}

void PS5Cemu_PresentRelease()
{
#ifdef PLAYTENDOH_DEBUG
	s_grab.Destroy();
	s_device = VK_NULL_HANDLE;
	s_swapchain = VK_NULL_HANDLE;
	s_ready = false;
#endif
}
