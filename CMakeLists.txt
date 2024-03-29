﻿# @file CMakeLists.txt
#
# CMake scripts for building applications.
# SPDX-License-Identifier: WTFPL

cmake_minimum_required (VERSION 3.16)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/3rdparty/CMakeModules")
# For MSVC
if (POLICY CMP0141)
  cmake_policy (SET CMP0141 NEW)
  set (CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("VulkanComputeRayTracing")

if(LINUX)
  set(PLATFORMS "x11"
   CACHE STRING "Use window system")
  set_property(CACHE PLATFORMS PROPERTY STRINGS
    ""
    "x11"
    "wayland"
    )
endif()

option(LOAD_SHADER_FROM_MEMORY ON)

if(WIN32)
  set (PLATFORM_SOURCE "platform/Win32.cpp")
elseif(LINUX)
  set (PLATFORM_SOURCE "platform/Linux.cpp")
endif()

add_executable (VulkanComputeRayTracing "VulkanComputeRayTracing.cpp" ${PLATFORM_SOURCE} "Environment.cpp" "Frontend.cpp" "Shader.cpp" "Renderer.cpp" )
include_directories (VulkanComputeRayTracing "include")
set (SHADER_SOURCES "shaders/shader.frag" "shaders/shader.vert" "shaders/shader.comp")

find_package(Vulkan)
if(Vulkan_FOUND)
  message (STATUS "Vulkan ${Vulkan_VERSION} headers found at ${Vulkan_INCLUDE_DIR}")
  target_include_directories (VulkanComputeRayTracing PRIVATE ${Vulkan_INCLUDE_DIR})
  message (STATUS "Vulkan ${Vulkan_VERSION} libraries found at ${Vulkan_LIBRARIES}")
  target_link_libraries (VulkanComputeRayTracing ${Vulkan_LIBRARIES})
else()
  message(FATAL_ERROR "Vulkan Not found!")
endif()

# Window System
if(LINUX)
  foreach(platform IN LISTS PLATFORMS)
    if(platform MATCHES "[Xx]11")
      find_package(X11)
      if(X11_FOUND)
        set(VCRT_PLATFORM_HAS_X11 ON)
        target_compile_definitions(VulkanComputeRayTracing PRIVATE VCRT_PLATFORM_HAS_X11)
      endif()
    elseif(platform MATCHES "[Ww]ayland")
      set(VCRT_PLATFORM_HAS_WAYLAND ON)
      target_compile_definitions(VulkanComputeRayTracing PRIVATE VCRT_PLATFORM_HAS_WAYLAND)
    endif()
  endforeach()
endif()

if(VCRT_PLATFORM_HAS_X11)
  find_package(XCB)
  if(XCB_FOUND)
    message (STATUS "XCB ${XCB_VERSION} headers found at ${XCB_INCLUDE_DIRS}")
    target_include_directories (VulkanComputeRayTracing PRIVATE ${XCB_INCLUDE_DIRS})
    message (STATUS "XCB ${XCB_VERSION} libraries found at ${XCB_XCB_LIBRARY}")
    target_link_libraries (VulkanComputeRayTracing ${XCB_XCB_LIBRARY})
  else()
    message(FATAL_ERROR "XCB Not found!")
  endif()
endif()

if(VCRT_PLATFORM_HAS_WAYLAND)
  find_package(Wayland)
  if(Wayland_Client_FOUND)
    message (STATUS "Wayland ${Wayland_Client_VERSION} headers found at ${Wayland_INCLUDE_DIRS}")
    target_include_directories (VulkanComputeRayTracing PRIVATE ${Wayland_INCLUDE_DIRS})
    message (STATUS "Wayland ${Wayland_Client_VERSION} libraries found at ${Wayland_Client_LIBRARY}")
    target_link_libraries (VulkanComputeRayTracing ${Wayland_Client_LIBRARY})
  else()
    message(FATAL_ERROR "Wayland Not found!")
  endif()

  find_package(WaylandScanner)
  if(WaylandScanner_FOUND)
    ecm_add_wayland_client_protocol(VulkanComputeRayTracing PROTOCOL ${CMAKE_SOURCE_DIR}/3rdparty/xdg-shell.xml BASENAME xdg-shell)
    target_include_directories (VulkanComputeRayTracing PRIVATE ${CMAKE_CURRENT_BINARY_DIR}) # Generated Header
  else()
    message(FATAL_ERROR "Wayland Scanner Not found!")
  endif()
endif()

if(Vulkan_GLSLC_EXECUTABLE)
  message (STATUS "GLSLC Executable: ${Vulkan_GLSLC_EXECUTABLE}")
  set(GLSL_SHADER_COMPILER ${Vulkan_GLSLC_EXECUTABLE})
  set(GLSL_SHADER_COMPILER_COMMON_OPTIONS
    --target-env=vulkan
    $<$<BOOL:${LOAD_SHADER_FROM_MEMORY}>:-mfmt=num>
  )
elseif(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
  message (STATUS "GLSLANG_VALIDATOR Executable: ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")
  set(GLSL_SHADER_COMPILER ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})
  set(GLSL_SHADER_COMPILER_COMMON_OPTIONS
    -V
    $<$<BOOL:${LOAD_SHADER_FROM_MEMORY}>:-x>
  )
else()
  message (FATAL_ERROR "glslc or glslangValidator Not found!")
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property (TARGET VulkanComputeRayTracing PROPERTY CXX_STANDARD 20)
endif()

if(LOAD_SHADER_FROM_MEMORY)
  set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
  target_compile_definitions(VulkanComputeRayTracing PRIVATE LOAD_SHADER_FROM_MEMORY)
  target_include_directories(VulkanComputeRayTracing PRIVATE ${SHADER_OUTPUT_DIR})
else()
  set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR})
endif()
foreach(source IN LISTS SHADER_SOURCES)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${GLSL_SHADER_COMPILER}
      ${GLSL_SHADER_COMPILER_COMMON_OPTIONS}
      -o ${SHADER_OUTPUT_DIR}/${FILENAME}.spv
      ${CMAKE_SOURCE_DIR}/${source}
    OUTPUT ${SHADER_OUTPUT_DIR}/${FILENAME}.spv
    DEPENDS ${CMAKE_BINARY_DIR}
    COMMENT "Compiling shader: ${FILENAME}"
  )
  list(APPEND SHADER_BINARIES ${SHADER_OUTPUT_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(compile-shaders ALL DEPENDS ${SHADER_BINARIES})
if(LOAD_SHADER_FROM_MEMORY)
  add_dependencies(VulkanComputeRayTracing compile-shaders)
endif()

# RandomGenerator
add_executable (SceneGenerator SceneGenerator.cpp)
