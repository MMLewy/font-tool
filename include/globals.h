#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

extern const char* const APPLICATION_NAME;
extern const uint32_t APPLICATION_VERSION;

#ifdef DEBUG
#define assert(condition) fn_assert((condition), "assert("#condition"); failed!", __FILE__, __LINE__)
void fn_assert(bool condition, const char* msg, const char* file, uint32_t line);
#else
#define assert(condition) ((void)0)
#endif

#endif