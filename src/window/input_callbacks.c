#include <window/input_callbacks.h>

#include <stdio.h>

void input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_REPEAT || key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST) return;

    if(key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) glfwSetWindowShouldClose(window, GLFW_TRUE);
    if(key == GLFW_KEY_Q && action == GLFW_RELEASE) glfwSetWindowAttrib(window, GLFW_DECORATED, !glfwGetWindowAttrib(window, GLFW_DECORATED));

    printf("KEY: %d %s %d MOD: 0x%X\n", key, glfwGetKeyName(key, scancode), scancode, mods);
}