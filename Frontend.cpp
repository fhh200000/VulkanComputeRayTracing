/* @file Frontend.hpp

    Implementations related to Vulkan frontend (Swapchain, ImageView,...).
    SPDX-License-Identifier: WTFPL

*/

#ifdef _MSC_VER
// Disables warnings about un-initialized array accesses.
// Arrays are filled in Vulkan call.
#pragma warning( disable : 6385 )
#endif

#include <Frontend.hpp>
#include <Environment.hpp>

static VkSurfaceCapabilitiesKHR vulkanSurfaceCapabilities;
static VkPresentModeKHR vulkanPresentMode;
static VkImage* vulkanSwapChainImages;
VkSwapchainKHR vulkanSwapChain;
VkImageView *vulkanSwapChainImageViews;
VkSurfaceFormatKHR vulkanSurfaceFormat;
uint32_t vulkanSwapChainImageCount;

static void chooseSwapSurfaceFormat(void)
{

    uint32_t formatCount;
    VkSurfaceFormatKHR* vulkanSurfaceFormats;

    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanPhysicalDevice, vulkanWindowSurface, &formatCount, nullptr);
    if (formatCount != 0) {
        vulkanSurfaceFormats = new VkSurfaceFormatKHR[formatCount];
        vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanPhysicalDevice, vulkanWindowSurface,
                                            &formatCount, vulkanSurfaceFormats);
    }
    else {
        return;
    }

    vulkanSurfaceFormat = vulkanSurfaceFormats[0];
    for (uint32_t iter = 0; iter < formatCount; ++iter) {
        if (vulkanSurfaceFormats[iter].format == VK_FORMAT_B8G8R8A8_SRGB
         && vulkanSurfaceFormats[iter].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            vulkanSurfaceFormat = vulkanSurfaceFormats[iter];
            break;
        }
    }

    delete[] vulkanSurfaceFormats;
    return;
}

static void chooseSwapPresentMode(void)
{

    VkPresentModeKHR *vulkanPresentModes;
    uint32_t vulkanPresentModeCount;

    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanPhysicalDevice, vulkanWindowSurface, &vulkanPresentModeCount, nullptr);
    if (vulkanPresentModeCount != 0) {
        vulkanPresentModes = new VkPresentModeKHR[vulkanPresentModeCount];
        vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanPhysicalDevice, vulkanWindowSurface, 
                                                 &vulkanPresentModeCount, vulkanPresentModes);
    }
    else {
        return;
    }
    vulkanPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t iter = 0; iter < vulkanPresentModeCount; ++iter) {
        if (vulkanPresentModes[iter] == VK_PRESENT_MODE_IMMEDIATE_KHR) { // We do NOT care about screen tearing!
            vulkanPresentMode = vulkanPresentModes[iter];
            break;
        }
    }

    delete[] vulkanPresentModes;
    return;
}

VkResult CreateVulkanWindowFrontend(void)
{

    VkResult result;
    VkSurfaceCapabilitiesKHR capabilities;
    VkExtent2D extent = {
        .width  = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT
    };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanPhysicalDevice, vulkanWindowSurface, &capabilities);
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.minImageCount;
    }
    chooseSwapSurfaceFormat();
    chooseSwapPresentMode();

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vulkanWindowSurface,
        .minImageCount = imageCount,
        .imageFormat = vulkanSurfaceFormat.format,
        .imageColorSpace = vulkanSurfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        // TODO: Cross-GPU sharing.
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = vulkanPresentMode,
        .clipped = VK_TRUE
    };
    
    result = vkCreateSwapchainKHR(vulkanLogicalDevice, &createInfo, nullptr, &vulkanSwapChain);
    if (result != VK_SUCCESS) {
        return result;
    }

    result = vkGetSwapchainImagesKHR(vulkanLogicalDevice, vulkanSwapChain, &imageCount, nullptr);
    if (result != VK_SUCCESS) {
        return result;
    }
    vulkanSwapChainImageCount = imageCount;
    vulkanSwapChainImages = new VkImage[imageCount];
    result = vkGetSwapchainImagesKHR(vulkanLogicalDevice, vulkanSwapChain, &imageCount, vulkanSwapChainImages);
    if (result != VK_SUCCESS) {
        return result;
    }

    vulkanSwapChainImageViews = new VkImageView[imageCount];
    VkImageViewCreateInfo imageViewcreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vulkanSurfaceFormat.format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    for (uint32_t iter = 0; iter < imageCount; ++iter) {
        imageViewcreateInfo.image = vulkanSwapChainImages[iter];
        result = vkCreateImageView(vulkanLogicalDevice, &imageViewcreateInfo, nullptr, &vulkanSwapChainImageViews[iter]);
        if (result != VK_SUCCESS) {
            return result;
        }
    }
    
    return VK_SUCCESS;
}

VkResult DestroyVulkanWindowFrontend(void)
{
    if (vulkanSwapChainImageViews != nullptr) {
        for (uint32_t iter = 0; iter < vulkanSwapChainImageCount; ++iter) {
            vkDestroyImageView(vulkanLogicalDevice,vulkanSwapChainImageViews[iter] , nullptr);
        }
        delete[] vulkanSwapChainImageViews;
    }
    if (vulkanSwapChain != nullptr) {
        vkDestroySwapchainKHR(vulkanLogicalDevice, vulkanSwapChain, nullptr);
    }
    if (vulkanSwapChainImages != nullptr) {
        delete[] vulkanSwapChainImages;
    }
    return VK_SUCCESS;
}
