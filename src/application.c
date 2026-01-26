#include <application.h>

#include <window/window.h>
#include <graphics/graphics.h>
#include <globals.h>

bool app_init()
{
    Graphics_error error = graphics_init();
    assert(error == GRAPHICS_OK);

    return true;
}

bool app_run()
{
    if(window_create("Test") != WINDOW_OK) return false;

    while(!window_should_close())
    {
        window_poll_events();
    }

    graphics_cleanup();
    window_destroy();

    return true;
}
