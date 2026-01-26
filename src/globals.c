#include <globals.h>



#define VK_MAKE_VERSION(major, minor, patch) \
    ((((uint32_t)(major)) << 22U) | (((uint32_t)(minor)) << 12U) | ((uint32_t)(patch)))

const char* const APPLICATION_NAME = "Test";
const uint32_t APPLICATION_VERSION = VK_MAKE_VERSION(1, 0, 0);


#ifdef DEBUG

#include <stdio.h>
#include <stdlib.h>

void fn_assert(bool condition, const char* msg, const char* file, uint32_t line)
{
    if(!condition)
    {
        printf("%s:%d %s", file, line, msg);
        exit(-1);
    }
}

#endif