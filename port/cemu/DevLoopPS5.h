// SPDX-License-Identifier: GPL-3.0-or-later
// Playtendoh: the Wii U side's frames as PS5_DevLoop grabs them, in a debug build (port/devloop/grab.h).
// Cemu's VulkanRenderer::SwapBuffer presents through PS5Cemu_Present (patches/cemu) with the TV's
// swapchain, so a grab is the TV as shown, Cemu's frame with whatever was drawn over it. In a release
// build it is vkQueuePresentKHR and nothing else. CemuCafe compiles it, on Cemu's Vulkan entry points.
#pragma once

#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanAPI.h"

#include <cstdint>
#include <vector>

// In place of vkQueuePresentKHR. tv: the main window's swapchain (the GamePad's is never grabbed);
// images, format and extent: that swapchain's, made with VK_IMAGE_USAGE_TRANSFER_SRC_BIT (SwapchainInfoVk).
VkResult PS5Cemu_Present(VkQueue queue, const VkPresentInfoKHR& info, bool tv, VkDevice device, VkPhysicalDevice physical,
	uint32_t queueFamily, const std::vector<VkImage>& images, VkFormat format, VkExtent2D extent);
// What it made for the device, before the device goes
void PS5Cemu_PresentRelease();
