/* @file Win32.cpp

    Implementataion of Win32 platform-specific operations.
    SPDX-License-Identifier: WTFPL

*/
#define UNICODE
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <Platform.hpp>
#include <Environment.hpp>
#include <vulkan/vulkan_win32.h>

static HWND mainWindowHwnd;
static HINSTANCE executableInstance;
const char* platformExtensions[] = {
    "VK_KHR_surface",
    "VK_KHR_win32_surface"
};
const uint32_t    platformExtensionCount = static_cast<uint32_t>(sizeof(platformExtensions) / sizeof(const char*));
constexpr LPCWSTR WindowClassName = L"VulkanComputeRayTracingClass";

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

VkResult PlatformCreateWindow(OUT VkSurfaceKHR* surface)
{
    executableInstance = GetModuleHandle(nullptr);
    WNDCLASS wc = {
        .lpfnWndProc = WindowProc,
        .hInstance = executableInstance,
        .lpszClassName = WindowClassName
    };
    RegisterClass(&wc);

    mainWindowHwnd = CreateWindowW(WindowClassName, reinterpret_cast<LPCWSTR>(applicationName),
                                   WS_CAPTION | WS_VISIBLE | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
                                   WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, executableInstance, nullptr);
    if (!mainWindowHwnd) {
        int error = GetLastError();
        LPCTSTR strErrorMessage = NULL;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL,error,0,(LPWSTR)&strErrorMessage,0,NULL);
        OutputDebugString(strErrorMessage);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkWin32SurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = executableInstance,
        .hwnd = mainWindowHwnd
    };

    return vkCreateWin32SurfaceKHR(vulkanInstance, &createInfo, nullptr, surface);
}

void PlatformEnterEventLoop(void)
{
    MSG msg;
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1) {
            return;
        }
        if (msg.message == WM_QUIT) {
            return;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}
