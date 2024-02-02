/* @file Environment.hpp

    Functions related to Vulkan & application runtime environment.
    SPDX-License-Identifier: WTFPL

*/

#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <Common.hpp>

extern VkInstance vulkanInstance;
extern VkDevice vulkanLogicalDevice;
extern VkSurfaceKHR vulkanWindowSurface;
extern VkPhysicalDevice vulkanPhysicalDevice;
extern uint32_t vulkanQueueFamilyIndex;
extern VkQueue vulkanGraphicsQueue;
extern VkQueue vulkanComputeQueue;

// Create vulkan runtime environment.
VkResult CreateVulkanRuntimeEnvironment(void);

// Create vulkan window environment.
VkResult CreateVulkanWindowEnvironment(void);

// Clean up vulkan runtime environment.
VkResult DestroyVulkanRuntimeEnvironment(void);

#endif
