/* @file Common.cpp

    Definition of common-used items.
    SPDX-License-Identifier: WTFPL

*/

#ifndef COMMON_HPP
#define COMMON_HPP

#define IN
#define OUT

#if !defined(NDEBUG)
#define DEBUG_INFORMATION
#endif

#include <cstdint>
#include <algorithm>
// Vulkan bug that we cannot use vulkan.hpp for C++.
#include <vulkan/vulkan.h>

constexpr auto WINDOW_WIDTH = 1280;
constexpr auto WINDOW_HEIGHT = 720;
constexpr auto RENDER_ITERATION = 100;

extern const char16_t *applicationName;
extern const char     *applicationNameNarrow;

#endif
