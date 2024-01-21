/* @file Environment.hpp

    Functions related to Vulkan & application runtime environment.
    SPDX-License-Identifier: WTFPL

*/
#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <Common.hpp>

extern VkInstance vulkanInstance;

// Create vulkan runtime environment.
VkResult CreateVulkanRuntimeEnvironment(void);

// Create vulkan window environment.
VkResult CreateVulkanWindowEnvironment(void);

// Clean up vulkan runtime environment.
VkResult DestroyVulkanRuntimeEnvironment(void);

#endif
