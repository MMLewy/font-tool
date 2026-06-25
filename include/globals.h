#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

extern const char* const APPLICATION_NAME;
extern const uint32_t APPLICATION_VERSION;

#define UNUSED(x) (void)x

#ifdef DEBUG

#include <stdio.h>

#define assert(condition) fn_assert((condition), "assert("#condition"); failed!", __FILE__, __LINE__)
void fn_assert(bool condition, const char* msg, const char* file, uint32_t line);
#define g_printf(...) printf(__VA_ARGS__)

#else

#define assert(condition) ((void)0)
#define g_printf(...) ((void)0)

#endif

#endif