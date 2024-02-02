/* @file Win32.cpp

    Definition of platform-specific operations.
    SPDX-License-Identifier: WTFPL

*/

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include <Common.hpp>
#include <Renderer.hpp>

// Platform-specific required extensions.
extern const char*    platformExtensions[];
extern const uint32_t platformExtensionCount;

// Create platform-specific window.
VkResult PlatformCreateWindow(OUT VkSurfaceKHR *surface);

// Enter platform-specific event loop.
void PlatformEnterEventLoop(void);

#endif
