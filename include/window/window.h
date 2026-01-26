#ifndef WINDOW_H
#define WINDOW_H

#include <window/window_errors.h>

void window_alert(char const *message);

Window_error window_create(const char *title);
void window_destroy();

bool window_should_close();
void window_poll_events();

#endif