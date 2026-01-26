#include <graphics/graphics.h>

#include <globals.h>
#include <volk.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{

    printf("validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

VkDebugUtilsMessengerEXT debug_messenger;

#endif


static VkInstance vulkan_instance;



Graphics_error graphics_init()
{
    if(volkInitialize() == VK_ERROR_INITIALIZATION_FAILED) return GRAPHICS_VULKAN_INITIALIZATION;

#ifdef DEBUG
    const char* desired_layers[] = 
    {
        "VK_LAYER_KHRONOS_validation",
    };
    uint32_t desired_layers_count = sizeof(desired_layers) / sizeof(char*);

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = NULL,
    };

#endif

    // TODO: Enhance in the future the extension selection on a different platforms. 
    const char* desired_extensions[] = 
    {
        "VK_KHR_surface",
#ifdef _WIN32
        "VK_KHR_win32_surface",
#elifdef __APPLE__
        "VK_EXT_metal_surface", // Other possible extension: VK_MVK_macos_surface
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#elifdef USE_WAYLAND
        "VK_KHR_wayland_surface",
#else
        "VK_KHR_xlib_surface", // Other possible extension: VK_KHR_xcb_surface
#endif

#ifdef DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };
    uint32_t desired_extensions_count = sizeof(desired_extensions) / sizeof(char*);

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);

    VkExtensionProperties* extensions = (VkExtensionProperties*)malloc(extension_count * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extensions);

    // TODO: Improve this loop
    uint32_t found_extension_count = 0;
    for(int i = 0; i < desired_extensions_count; i++)
    {
        for(int j = 0; j < extension_count; j++)
        {
            if(strcmp(desired_extensions[i], extensions[j].extensionName) == 0)
            {
                ++found_extension_count;
            }
        }
    }
    free(extensions);

    if(found_extension_count != desired_extensions_count) return GRAPHICS_VULKAN_PRESENTATION_EXTENSIONS;

    VkApplicationInfo app_info = 
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = APPLICATION_NAME,
        .applicationVersion = APPLICATION_VERSION,
        .pEngineName = APPLICATION_NAME,
        .engineVersion = APPLICATION_VERSION,
        .apiVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo instance_info =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef __APPLE__
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#else
        .flags = 0,
#endif
        .pApplicationInfo = &app_info,
#ifdef DEBUG
        .pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_messenger_create_info,
        .enabledLayerCount = desired_layers_count,
        .ppEnabledLayerNames = desired_layers,
#else
        .pNext = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
#endif
        .enabledExtensionCount = desired_extensions_count,
        .ppEnabledExtensionNames = desired_extensions,

    };

    VkResult result = vkCreateInstance(&instance_info, NULL, &vulkan_instance);

    if(result != VK_SUCCESS || vulkan_instance == NULL) return GRAPHICS_VULKAN_INSTANCE_CREATE;

    volkLoadInstanceOnly(vulkan_instance);

#ifdef DEBUG
    result = vkCreateDebugUtilsMessengerEXT(vulkan_instance, &debug_messenger_create_info, NULL, &debug_messenger);
    assert(result == VK_SUCCESS);
#endif

    return GRAPHICS_OK;
}

void graphics_cleanup()
{
#ifdef DEBUG
    vkDestroyDebugUtilsMessengerEXT(vulkan_instance, debug_messenger, NULL);
#endif

    vkDestroyInstance(vulkan_instance, NULL);
}
