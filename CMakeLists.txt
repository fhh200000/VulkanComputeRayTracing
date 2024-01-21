# @file CMakeLists.txt
#
# CMake scripts for building applications.
# SPDX-License-Identifier: WTFPL

cmake_minimum_required (VERSION 3.8)

# For MSVC
if (POLICY CMP0141)
  cmake_policy (SET CMP0141 NEW)
  set (CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("VulkanComputeRayTracing")

if(WIN32)
  set (PLATFORM_SOURCE "platform/Win32.cpp")
elseif(LINUX)
  set (PLATFORM_SOURCE "platform/Linux.cpp")
endif()

add_executable (VulkanComputeRayTracing "VulkanComputeRayTracing.cpp" ${PLATFORM_SOURCE} "Environment.cpp" "Frontend.cpp")
include_directories (VulkanComputeRayTracing "include")

find_package(Vulkan)
if(Vulkan_FOUND)
  message (STATUS "Vulkan ${Vulkan_VERSION} headers found at ${Vulkan_INCLUDE_DIR}")
  target_include_directories (VulkanComputeRayTracing PRIVATE ${Vulkan_INCLUDE_DIR})
  message (STATUS "Vulkan ${Vulkan_VERSION} libraries found at ${Vulkan_LIBRARIES}")
  target_link_libraries (VulkanComputeRayTracing ${Vulkan_LIBRARIES})
else()
  message(FATAL_ERROR "Vulkan Not found!")
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property (TARGET VulkanComputeRayTracing PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
