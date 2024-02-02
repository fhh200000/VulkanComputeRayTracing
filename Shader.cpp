/* @file Shader.hpp

    Implementation of utility function to load & bind shader files to a specific stage.
    SPDX-License-Identifier: WTFPL

*/

#if (defined __STDC_LIB_EXT1__ || _MSC_VER > 1400)
#define __STDC_WANT_LIB_EXT1__  // For fopen_s
#else
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename), (mode)))==NULL
#define fread_s(buffer,bufferSize,elementSize,count,stream) fread(buffer,elementSize,count,stream)
#endif

#include <Shader.hpp>
#include <cstdio>
#include <Environment.hpp>
#include <sys/stat.h>

VkResult CreateShaderStageFromFile(IN const char* filename, IN VkShaderStageFlagBits stage,
    OUT VkPipelineShaderStageCreateInfo* shaderStageCreateInfo)
{

    VkResult result;
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

    VkShaderModule shaderModule;
    result = vkCreateShaderModule(vulkanLogicalDevice, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        return result;
    }
    
    shaderStageCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo->stage = stage;
    shaderStageCreateInfo->module = shaderModule;
    shaderStageCreateInfo->pName = "main";

    delete[] fileData;
    return VK_SUCCESS;
}
