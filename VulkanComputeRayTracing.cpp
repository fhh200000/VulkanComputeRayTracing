/* @file VulkanComputeRayTracing.cpp

    Entry point for the main program.
    SPDX-License-Identifier: WTFPL

*/

#include <VulkanComputeRayTracing.hpp>

using std::cout;
using std::cerr;
using std::endl;

const char16_t* applicationName = u"Vulkan Compute Raytracing";
const char*     applicationNameNarrow = "Vulkan Compute Raytracing";

int main()
{
    cout << "Hello Vulkan." << endl;
    if (CreateVulkanRuntimeEnvironment() != VK_SUCCESS) {
        cerr << "Cannot create Vulkan runtime environment." << endl;
        return -1;
    }
    if (CreateVulkanWindowEnvironment() != VK_SUCCESS) {
        cerr << "Cannot create Vulkan window environment." << endl;
        return -1;
    }
    if (CreateVulkanWindowFrontend() != VK_SUCCESS) {
        cerr << "Cannot create Vulkan window frontend." << endl;
        return -1;
    }
    if (BeginRenderingOperation() != VK_SUCCESS) {
        cerr << "Cannot begin rendering operation." << endl;
        return -1;
    }
    PlatformEnterEventLoop();
    EndRenderingOperation();
    DestroyVulkanWindowFrontend();
    DestroyVulkanRuntimeEnvironment();
    cout << "Bye Vulkan." << endl;
    return 0;
}
