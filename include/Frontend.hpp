/* @file Frontend.hpp

    Functions related to Vulkan frontend (Swapchain, ImageView,...).
    SPDX-License-Identifier: WTFPL

*/

#ifndef FRONTEND_HPP
#define FRONTEND_HPP

#include <Common.hpp>

extern VkSwapchainKHR vulkanSwapChain;
extern VkSurfaceFormatKHR vulkanSurfaceFormat;
extern uint32_t vulkanSwapChainImageCount;
extern VkImageView* vulkanSwapChainImageViews;

// Create vulkan frontend.
VkResult CreateVulkanWindowFrontend(void);

// Destroy vulkan frontend.
VkResult DestroyVulkanWindowFrontend(void);

#endif
