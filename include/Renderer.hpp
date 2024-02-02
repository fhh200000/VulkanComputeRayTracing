/* @file Renderer.hpp

    Main renderer program that takes charge of rendering.
    SPDX-License-Identifier: WTFPL

*/

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <Common.hpp>

// Create pipeline, submit tasks...
VkResult BeginRenderingOperation(void);

// Draw next frame, to be called by platform handlers.
VkResult DrawNextFrame(void);

// End rendering & destroy allocated environments.
VkResult EndRenderingOperation(void);

#endif