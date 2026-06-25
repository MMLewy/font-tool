#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <graphics/graphics_errors.h>
#include <stdint.h>

typedef enum Graphics_state_machine_t {

    /// @brief Vulkan pre-initialization. 
    GRAPHICS_SM_PRE_INIT,

    /// @brief Afer Vulkan lib is loaded, window has to be created, in order to further initialize graphics module.
    GRAPHICS_SM_WAIT_FOR_WINDOW_CREATION,

    GRAPHICS_SM_INIT,

    GRAPHICS_SM_POST_INIT,

    GRAPHICS_SM_ERROR,


} Graphics_state_machine;

typedef union Graphics_notification_t {

    uint16_t value;

    struct {
        uint16_t window_created : 1;
    };

} Graphics_notification;

Graphics_error graphics_state_machine_loop();
Graphics_state_machine graphics_sm_get_state();
void graphics_sm_notify(Graphics_notification notification);
void graphics_cleanup();

#endif