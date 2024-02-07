/* @file Linux.cpp

    Implementataion of Linux platform-specific operations.
    SPDX-License-Identifier: WTFPL

*/

#include <Platform.hpp>
#include <Environment.hpp>
#include <tuple>

#if defined(VCRT_PLATFORM_HAS_X11) || defined(VCRT_PLATFORM_HAS_WAYLAND)
#include <cassert>
#include <cstdlib>
#include <cstring>
#endif

#if defined(VCRT_PLATFORM_HAS_X11)
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#endif

#if defined(VCRT_PLATFORM_HAS_WAYLAND)
#include <wayland-client.h>
#include <wayland-xdg-shell-client-protocol.h>
#include <vulkan/vulkan_wayland.h>
#endif

const char* platformExtensions[] = {
    "VK_KHR_surface",
#if defined(VCRT_PLATFORM_HAS_X11)
    "VK_KHR_xcb_surface",
#endif
#if defined(VCRT_PLATFORM_HAS_WAYLAND)
    "VK_KHR_wayland_surface"
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
#if defined(VCRT_PLATFORM_HAS_WAYLAND)
    wl_display              *wl_display_instance;
    wl_compositor           *wl_compositor_instance;
    wl_surface              *wl_surface_instance;
    xdg_wm_base             *wl_xdg_shell;
    xdg_surface             *wl_xdg_shell_surface;
    xdg_toplevel            *wl_xdg_toplevel;
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

static void handleShellPing(void* data, struct xdg_wm_base* shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);

    static_cast<void>(data);
}

static struct xdg_wm_base_listener shellListener = {
    .ping = handleShellPing
};

static void registryHandler(void *data, struct wl_registry *registry, uint32_t id,
                            const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0) {
        winSys.wl_compositor_instance = reinterpret_cast<wl_compositor *>(wl_registry_bind(registry,
                                            id, &wl_compositor_interface, 1));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        winSys.wl_xdg_shell = reinterpret_cast<xdg_wm_base*>(wl_registry_bind(registry,
                                            id, &xdg_wm_base_interface, 1));
        xdg_wm_base_add_listener(winSys.wl_xdg_shell, &shellListener, NULL);
    }

    static_cast<void>(data);
    static_cast<void>(version);
}

static const struct wl_registry_listener registryListener = {
    registryHandler
};

static void handleShellSurfaceConfigure(void* data, struct xdg_surface* shellSurface, uint32_t serial)
{
    xdg_surface_ack_configure(shellSurface, serial);

    static_cast<void>(data);
}

static const struct xdg_surface_listener shellSurfaceListener = {
    .configure = handleShellSurfaceConfigure
};

static void handleToplevelClose(void* data, struct xdg_toplevel* toplevel)
{
    winSys.quit = 1;

    static_cast<void>(data);
    static_cast<void>(toplevel);
}

static void handleToplevelConfigure(void *data, struct xdg_toplevel *xdg_toplevel,
                        int32_t width, int32_t height,  struct wl_array *states)
{
    static_cast<void>(data);
    static_cast<void>(xdg_toplevel);
    static_cast<void>(width);
    static_cast<void>(height);
    static_cast<void>(states);
}

static xdg_toplevel_listener toplevelListener = {
    .configure = handleToplevelConfigure,
    .close = handleToplevelClose
};

VkResult PlatformCreateWaylandWindow(OUT VkSurfaceKHR *surface)
{
    winSys.wl_display_instance = wl_display_connect(nullptr);
    if (winSys.wl_display_instance == nullptr) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    struct wl_registry *registry = wl_display_get_registry(winSys.wl_display_instance);
    wl_registry_add_listener(registry, &registryListener, nullptr);
    wl_display_roundtrip(winSys.wl_display_instance);
    if (winSys.wl_compositor_instance == nullptr) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    winSys.wl_surface_instance = wl_compositor_create_surface(winSys.wl_compositor_instance);
    if (winSys.wl_surface_instance == nullptr) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    winSys.wl_xdg_shell_surface = xdg_wm_base_get_xdg_surface(winSys.wl_xdg_shell, winSys.wl_surface_instance);

    if (winSys.wl_xdg_shell_surface == nullptr) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }
    winSys.wl_xdg_toplevel = xdg_surface_get_toplevel(winSys.wl_xdg_shell_surface);

    if (winSys.wl_xdg_toplevel == nullptr) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    xdg_surface_add_listener(winSys.wl_xdg_shell_surface, &shellSurfaceListener, nullptr);

    xdg_toplevel_set_title(winSys.wl_xdg_toplevel, applicationNameNarrow);
    xdg_toplevel_set_app_id(winSys.wl_xdg_toplevel, applicationNameNarrow);
    xdg_toplevel_set_max_size(winSys.wl_xdg_toplevel, WINDOW_WIDTH, WINDOW_HEIGHT);
    xdg_toplevel_set_min_size(winSys.wl_xdg_toplevel, WINDOW_WIDTH, WINDOW_HEIGHT);
    xdg_toplevel_add_listener(winSys.wl_xdg_toplevel,&toplevelListener, nullptr);

    wl_surface_commit(winSys.wl_surface_instance);
    wl_display_roundtrip(winSys.wl_display_instance);
    wl_surface_commit(winSys.wl_surface_instance);

    VkWaylandSurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = winSys.wl_display_instance,
        .surface = winSys.wl_surface_instance
    };
    return vkCreateWaylandSurfaceKHR(vulkanInstance,&createInfo,nullptr, surface);
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
    // No need to show explicitly.
}

void PlatformHandleWaylandEvent(void)
{
    wl_display_roundtrip(winSys.wl_display_instance);
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
        return std::make_tuple(&PlatformShowWaylandWindow,
                              &PlatformHandleWaylandEvent);
    }
    assert(false && "Unknown window system.");
    return std::make_tuple(nullptr, nullptr);
#elif defined(VCRT_PLATFORM_HAS_X11)
    return std::make_tuple(&PlatformShowXWindow, &PlatformHandleXEvent);
#elif defined(VCRT_PLATFORM_HAS_WAYLAND)
    return std::make_tuple(&PlatformShowWaylandWindow,
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
