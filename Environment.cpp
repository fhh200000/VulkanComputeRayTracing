/* @file Environment.hpp

    Implementations related to Vulkan & application runtime environment.
    SPDX-License-Identifier: WTFPL

*/

#ifdef _MSC_VER
// Disables warnings about un-initialized array accesses.
// Arrays are filled in Vulkan call.
#pragma warning( disable : 6385 )
#endif


#include <Environment.hpp>
#include <Platform.hpp>
#include <iostream>

using std::cout;
using std::endl;

VkInstance vulkanInstance;
VkDevice vulkanLogicalDevice;
VkQueue vulkanGraphicsQueue;
VkQueue vulkanComputeQueue;
VkPhysicalDevice vulkanPhysicalDevice;
VkSurfaceKHR     vulkanWindowSurface;
uint32_t vulkanQueueFamilyIndex;

#ifdef DEBUG_INFORMATION
const char*        enabledLayers[] = {
    "VK_LAYER_KHRONOS_validation",
    //"VK_LAYER_LUNARG_api_dump",
    "VK_LAYER_LUNARG_monitor"
};
const uint32_t enabledLayerCount = static_cast<uint32_t>(sizeof(enabledLayers) / sizeof(const char*));
#else
const char** enabledLayers;
const uint32_t enabledLayerCount = 0;
#endif

const char* enabledExtensions[] = {
    "VK_KHR_swapchain"
};
const uint32_t enabledExtensionCount = static_cast<uint32_t>(sizeof(enabledExtensions) / sizeof(const char*));

VkResult CreateVulkanRuntimeEnvironment(void)
{

    VkResult result;

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = applicationNameNarrow,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = applicationNameNarrow,
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = enabledLayerCount,
        .ppEnabledLayerNames = enabledLayers,
        .enabledExtensionCount = platformExtensionCount,
        .ppEnabledExtensionNames = platformExtensions
    };

    result = vkCreateInstance(&createInfo, nullptr, &vulkanInstance);
    if (result != VK_SUCCESS) {
        return result;
    }
    return VK_SUCCESS;
}

static VkResult getDeviceQueueIndexByName(IN VkPhysicalDevice device, IN uint32_t flags, OUT uint32_t *index)
{
    VkResult result = VK_ERROR_FORMAT_NOT_SUPPORTED;
    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties *properties;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    if (!queueFamilyCount) {
        return result;
    }

    properties = new VkQueueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, properties);
    for (uint32_t iter = 0; iter < queueFamilyCount; ++iter) {
        if ((properties[iter].queueFlags & flags) == flags) {
            *index = iter;
            result = VK_SUCCESS;
            break;
        }
    }

    delete[] properties;
    return result;
}

VkResult CreateVulkanWindowEnvironment(void)
{
    VkResult result;
    uint32_t deviceCount;
    VkPhysicalDevice *physicalDevices;
    
    result = vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);
    if (result != VK_SUCCESS || deviceCount<=0) {
        return result;
    }
    physicalDevices = new VkPhysicalDevice[deviceCount];
    result = vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, physicalDevices);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Physical device selection policy:
    // If there are more than 1 GPUs, select the first dGPU.
    VkPhysicalDeviceProperties deviceProperties;
    uint32_t queueFamilyCount=0;
    for (uint32_t iter = 0; iter < deviceCount; ++iter) {
        vkGetPhysicalDeviceProperties(physicalDevices[iter], &deviceProperties);
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            vulkanPhysicalDevice = physicalDevices[iter];
            goto selection_done;
        }
    }
    // No dGPUs, select the first GPU.
    vulkanPhysicalDevice = physicalDevices[0];
    vkGetPhysicalDeviceProperties(vulkanPhysicalDevice, &deviceProperties);
selection_done:
    // Display GPU name & free buffer.
#ifdef DEBUG_INFORMATION
    cout << "Selected GPU device: " << deviceProperties.deviceName
         << ((deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? " (dGPU)" : " (iGPU)") << endl;
#endif
    delete[] physicalDevices;

    // Create VkDevice.

    float queuePriority[] = { 0.05f, 0.95f }; // Less graphics, more compute! 
    if (getDeviceQueueIndexByName(vulkanPhysicalDevice, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, &vulkanQueueFamilyIndex) != VK_SUCCESS) {
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vulkanQueueFamilyIndex,
        .queueCount = 2,
        .pQueuePriorities = queuePriority
    };

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = enabledExtensionCount,
        .ppEnabledExtensionNames = enabledExtensions,
        .pEnabledFeatures = &deviceFeatures,
    };

    result = vkCreateDevice(vulkanPhysicalDevice, &createInfo, nullptr, &vulkanLogicalDevice);
    if (result != VK_SUCCESS) {
        return result;
    }
    vkGetDeviceQueue(vulkanLogicalDevice, vulkanQueueFamilyIndex, 0, &vulkanGraphicsQueue);
    vkGetDeviceQueue(vulkanLogicalDevice, vulkanQueueFamilyIndex, 1, &vulkanComputeQueue);

    // Create Window.
    result = PlatformCreateWindow(&vulkanWindowSurface);
    if (result != VK_SUCCESS) {
        return result;
    }
    return VK_SUCCESS;
}

VkResult DestroyVulkanRuntimeEnvironment(void)
{
    vkDestroySurfaceKHR(vulkanInstance, vulkanWindowSurface, nullptr);
    vkDestroyDevice(vulkanLogicalDevice, nullptr);
    vkDestroyInstance(vulkanInstance, nullptr);
    return VK_SUCCESS;
}
