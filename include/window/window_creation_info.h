#ifndef WINDOW_CREATION_INFO
#define WINDOW_CREATION_INFO

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


typedef struct Window_creation_info_t
{
#ifdef _WIN32

HINSTANCE           hinstance;
HWND                hwnd;

#elif defined(__APPLE__)

const void*         pView;

#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)

struct wl_display*  display;
struct wl_surface*  surface;

#else 

// TODO: MID_PRIO Change from Xlib to Xcb after GLFW 3.5 release.
Display*            dpy;
Window              window;

#endif

} Window_creation_info;

Window_creation_info window_get_creation_info();

#endif