/* @file Linux.cpp

    Implementataion of Linux platform-specific operations.
    SPDX-License-Identifier: WTFPL

*/

#include <Platform.hpp>
#include <Environment.hpp>

#if defined(VCRT_PLATFORM_HAS_X11) && defined(VCRT_PLATFORM_HAS_WAYLAND)
#include <cassert>
#include <cstdlib>
#include <cstring>
#endif

#if defined(VCRT_PLATFORM_HAS_X11)
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#endif

#include <tuple>

const char* platformExtensions[] = {
    "VK_KHR_surface",
#if defined(VCRT_PLATFORM_HAS_X11)
    "VK_KHR_xcb_surface",
#endif
};

const uint32_t platformExtensionCount = static_cast<uint32_t>(sizeof(platformExtensions) / sizeof(const char*));

namespace {

using CreateWindowT = VkResult (*)(OUT VkSurfaceKHR *surface);
using ShowWindowT = void (*)(void);
using EventLoopT = void (*)(void);

struct WinSysInfo {
#if defined(VCRT_PLATFORM_HAS_X11)
    xcb_connection_t        *xcb_connection;
    xcb_screen_t            *xcb_screen;
    xcb_drawable_t           xcb_win;
    xcb_generic_event_t     *xcb_event;
    xcb_intern_atom_reply_t *xcb_atom_wm_delete_window;
#endif
    bool quit  = false;
    uint32_t width  = WINDOW_WIDTH;
    uint32_t height = WINDOW_HEIGHT;
} winSys;

#if defined(VCRT_PLATFORM_HAS_X11)
VkResult PlatformCreateXWindow(OUT VkSurfaceKHR *surface)
{
    int scr;
    winSys.xcb_connection = xcb_connect(NULL, &scr);
    const xcb_setup_t* setup = xcb_get_setup(winSys.xcb_connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);
    winSys.xcb_screen = iter.data;

    /* create black (foreground) graphic context */
    winSys.xcb_win = xcb_generate_id(winSys.xcb_connection);
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[2] = {
        winSys.xcb_screen->black_pixel,
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY
    };
    xcb_create_window(winSys.xcb_connection,           /* connection    */
                      XCB_COPY_FROM_PARENT,            /* depth         */
                      winSys.xcb_win,                  /* window Id     */
                      winSys.xcb_screen->root,         /* parent window */
                      0, 0,                            /* x, y          */
                      winSys.width, winSys.height,     /* width, height */
                      0,                               /* border_width  */
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,   /* class         */
                      winSys.xcb_screen->root_visual,  /* visual        */
                      mask, values);                   /* masks         */

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie =
        xcb_intern_atom(winSys.xcb_connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply =
        xcb_intern_atom_reply(winSys.xcb_connection, cookie, 0);
    xcb_intern_atom_cookie_t cookie2 =
        xcb_intern_atom(winSys.xcb_connection, 0, 16, "WM_DELETE_WINDOW");
    winSys.xcb_atom_wm_delete_window =
        xcb_intern_atom_reply(winSys.xcb_connection, cookie2, 0);
    xcb_change_property(winSys.xcb_connection, XCB_PROP_MODE_REPLACE,
                        winSys.xcb_win,
                        (*reply).atom, 4, 32, 1,
                        &(*winSys.xcb_atom_wm_delete_window).atom);
    free(reply);

    VkXcbSurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = winSys.xcb_connection,
        .window = winSys.xcb_win,
    };

    return vkCreateXcbSurfaceKHR(vulkanInstance, &createInfo, nullptr, surface);
}
#endif

#if defined(VCRT_PLATFORM_HAS_WAYLAND)
VkResult PlatformCreateWaylandWindow(OUT VkSurfaceKHR *surface)
{
    return VK_SUCCESS;
}
#endif

CreateWindowT WindowsSystemDispatch()
{
#if defined(VCRT_PLATFORM_HAS_X11) && defined(VCRT_PLATFORM_HAS_WAYLAND)
    char const* const session = getenv("XDG_SESSION_TYPE");
    assert(session != nullptr);
    if (strcmp(session, "x11") == 0) {
        return &PlatformCreateXWindow;
    } else if (strcmp(session, "wayland") == 0) {
        return &PlatformCreateWaylandWindow;
    }
    assert(false && "Unknown window system.");
    return nullptr;
#elif defined(VCRT_PLATFORM_HAS_X11)
    return &PlatformCreateXWindow;
#elif defined(VCRT_PLATFORM_HAS_WAYLAND)
    return &PlatformCreateWaylandWindow;
#else
#error "Unknown window system."
#endif
}

#if defined(VCRT_PLATFORM_HAS_X11)
void PlatformShowXWindow(void)
{
    xcb_map_window(winSys.xcb_connection, winSys.xcb_win);
    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = {100, 100};
    xcb_configure_window(winSys.xcb_connection, winSys.xcb_win,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
    xcb_flush(winSys.xcb_connection);
}

void PlatformHandleXEvent(void)
{
    xcb_generic_event_t* event = xcb_poll_for_event(winSys.xcb_connection);
    while (event) {
        uint8_t event_code = event->response_type & 0x7f;
        switch (event_code) {
            case XCB_EXPOSE:
                // TODO: Resize window
                break;
            case XCB_CLIENT_MESSAGE:
                if ((*reinterpret_cast<xcb_client_message_event_t*>(event))
                        .data.data32[0] ==
                    (*winSys.xcb_atom_wm_delete_window).atom) {
                    winSys.quit = true;
                }
                break;
            case XCB_KEY_RELEASE: {
                auto* key =
                    reinterpret_cast<const xcb_key_release_event_t*>(event);
                switch (key->detail) {
                    case 0x9: // Escape
                        winSys.quit = true;
                        break;
                }
                break;
            }
            default:
                break;
        }
        free(event);
        event = xcb_poll_for_event(winSys.xcb_connection);
    }
}
#endif

#if defined(VCRT_PLATFORM_HAS_WAYLAND)
void PlatformShowWaylandWindow(void)
{

}

void PlatformHandleWaylandEvent(void)
{
    winSys.quit = true;
}
#endif

auto WindowSystemEventDispatch()
    -> std::tuple<ShowWindowT, EventLoopT>
{
#if defined(VCRT_PLATFORM_HAS_X11) && defined(VCRT_PLATFORM_HAS_WAYLAND)
    char const* const session = getenv("XDG_SESSION_TYPE");
    assert(session != nullptr);
    if (strcmp(session, "x11") == 0) {
        return std::make_tuple(&PlatformShowXWindow, &PlatformHandleXEvent);
    } else if (strcmp(session, "wayland") == 0) {
        returnstd::make_tuple(&PlatformShowWaylandWindow,
                              &PlatformHandleWaylandEvent);
    }
    assert(false && "Unknown window system.");
    return nullptr;
#elif defined(VCRT_PLATFORM_HAS_X11)
    return std::make_tuple(&PlatformShowXWindow, &PlatformHandleXEvent);
#elif defined(VCRT_PLATFORM_HAS_WAYLAND)
    returnstd::make_tuple(&PlatformShowWaylandWindow,
                          &PlatformHandleWaylandEvent);
#else
#error "Unknown window system."
#endif
}

}

VkResult PlatformCreateWindow(OUT VkSurfaceKHR *surface)
{
    CreateWindowT create_window = WindowsSystemDispatch();
    return create_window(surface);
}

void PlatformEnterEventLoop(void)
{
    auto [showWindow, handleEvent] = WindowSystemEventDispatch();

    showWindow();

    while (!winSys.quit) {
        DrawNextFrame();
        handleEvent();
        vkDeviceWaitIdle(vulkanLogicalDevice);
    }
}
