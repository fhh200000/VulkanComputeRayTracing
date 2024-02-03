/* @file Shader.hpp

    Implementation of utility function to load & bind shader files to a specific stage.
    SPDX-License-Identifier: WTFPL

*/

#include <Shader.hpp>
#include <cstdint>
#include <cstdio>
#include <Environment.hpp>

#if defined(LOAD_SHADER_FROM_MEMORY)
#include <cstring>
static const uint32_t vertSpirv[] = {
#include "shader.vert.spv"
};
static const uint32_t fragSpirv[] = {
#include "shader.frag.spv"
};
static const uint32_t compSpirv[] = {
#include "shader.comp.spv"
};
#else
#if (defined __STDC_LIB_EXT1__ || _MSC_VER > 1400)
#define __STDC_WANT_LIB_EXT1__  // For fopen_s
#else
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename), (mode)))==NULL
#define fread_s(buffer,bufferSize,elementSize,count,stream) fread(buffer,elementSize,count,stream)
#endif
#include <sys/stat.h>
#endif

VkResult CreateShaderStageFromFile(IN const char* filename, IN VkShaderStageFlagBits stage,
    OUT VkPipelineShaderStageCreateInfo* shaderStageCreateInfo)
{

    VkResult result;

#if defined(LOAD_SHADER_FROM_MEMORY)
    const uint32_t* spirv = nullptr;
    size_t codeSize = 0;
    if (strcmp(filename, "shader.vert.spv") == 0) {
        codeSize = sizeof(vertSpirv);
        spirv = &vertSpirv[0];
    } else if (strcmp(filename, "shader.frag.spv") == 0) {
        codeSize = sizeof(fragSpirv);
        spirv = &fragSpirv[0];
    } else if (strcmp(filename, "shader.comp.spv") == 0) {
        codeSize = sizeof(compSpirv);
        spirv = &compSpirv[0];
    }
    VkShaderModuleCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode = spirv,
    };
#else
    struct stat fileInfo;
    if (stat(filename, &fileInfo)) {
        return VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT;
    }
    uint32_t* fileData = new uint32_t [fileInfo.st_size/sizeof(uint32_t)];

    FILE* file;
    if (fopen_s(&file, filename, "rb")) {
        return VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT;
    }

    fread_s(fileData, fileInfo.st_size, fileInfo.st_size, 1, file);
    fclose(file);

    VkShaderModuleCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<size_t>(fileInfo.st_size),
        .pCode = fileData
    };
#endif

    VkShaderModule shaderModule;
    result = vkCreateShaderModule(vulkanLogicalDevice, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        return result;
    }
    
    shaderStageCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo->stage = stage;
    shaderStageCreateInfo->module = shaderModule;
    shaderStageCreateInfo->pName = "main";

#if !defined(LOAD_SHADER_FROM_MEMORY)
    delete[] fileData;
#endif
    return VK_SUCCESS;
}
