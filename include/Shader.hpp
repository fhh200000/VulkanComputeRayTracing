/* @file Shader.hpp

    Utility function to load & bind shader files to a specific stage.
    SPDX-License-Identifier: WTFPL

*/

#ifndef SHADER_HPP
#define SHADER_HPP

#include <Common.hpp>

// Load & bind.
VkResult CreateShaderStageFromFile (IN const char* filename, IN VkShaderStageFlagBits stage, 
                                    OUT VkPipelineShaderStageCreateInfo* shaderStageCreateInfo);

#endif