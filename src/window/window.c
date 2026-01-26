#include <window/window.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <window/input_callbacks.h>

#include <stdio.h>

static GLFWwindow *wnd;
static bool initialized = false;

void monitor_scan()
{
    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);

    if(count == 0) return;

    for(int i = 0; i < count; i++)
    {
        printf("NAME: %s\nMODES: \n", glfwGetMonitorName(monitors[i]));
        int count2 = 0;
        const GLFWvidmode* modes = glfwGetVideoModes(monitors[i], &count2);

        for(int j = 0; j < count2; j++)
        {
            printf("%dx%d %dHz\n", modes[j].width, modes[j].height, modes[j].refreshRate);
        }

        int xpos, ypos, width, height;
        glfwGetMonitorWorkarea(monitors[i], &xpos, &ypos, &width, &height);

        printf("POSITION: %d %d %dx%d\n", xpos, ypos, width, height);
    }
}


Window_error window_create(const char *title)
{
    initialized = glfwInit() == GLFW_TRUE;

    if(!initialized) return WINDOW_GLFW_INIT;

    monitor_scan();
    
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    wnd = glfwCreateWindow(800, 600, title, NULL, NULL);

    if(wnd == NULL) return WINDOW_WINDOW_CREATE;

    glfwSetInputMode(wnd, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
    glfwSetKeyCallback(wnd, input_key_callback);

    return WINDOW_OK;
}

void window_destroy()
{
    if(wnd != NULL)
    {
        glfwDestroyWindow(wnd);
        wnd = NULL;
    }

    if (initialized)
    {
        glfwTerminate();
        initialized = false;
    }
}

inline bool window_should_close()
{
    return glfwWindowShouldClose(wnd) == GLFW_TRUE;
}

inline void window_poll_events()
{
    glfwPollEvents();
}

#ifdef _WIN32
void window_alert(char const *message)
{
    MessageBox(NULL, message, "Critical error", MB_TOPMOST | MB_ICONERROR | MB_OK);
}
#elifdef __APPLE__

#else

#endif