#include <application.h>

#include <window/window.h>
#include <graphics/graphics.h>
#include <globals.h>

bool app_init()
{
    Graphics_notification notification = {};

    if(window_create("Test") != WINDOW_OK) return false;
    notification.window_created = 1;
    graphics_sm_notify(notification);

    Graphics_error error = GRAPHICS_OK;
    while(graphics_sm_get_state() != GRAPHICS_SM_RENDER)
    {
        error = graphics_state_machine_loop();
        if(error != GRAPHICS_OK) break;
    }

    // TODO: MID_PRIO Do proper error handling. Maybe alert window with error information.
    if(error != GRAPHICS_OK) return false;

    return true;
}

bool app_run()
{
    while(!window_should_close())
    {
        window_poll_events();
    }

    graphics_cleanup();
    window_destroy();

    return true;
}
