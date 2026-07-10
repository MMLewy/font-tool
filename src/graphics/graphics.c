#include <graphics/graphics.h>

#include <window/window_creation_info.h>
#include <globals.h>
#include <volk.h>
#include <stdlib.h>
#include <string.h>

// Temporary
#define SETTINGS_WIDTH 800
#define SETTINGS_HEIGHT 600

typedef struct Graphics_queue_properties_t
{
    uint32_t index;
    VkQueueFamilyProperties properties; 

} Graphics_queue_properties;

#ifdef DEBUG

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    UNUSED(messageSeverity);
    UNUSED(messageType);
    UNUSED(pUserData);

    g_printf("validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info =
{
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debug_callback,
    .pUserData = NULL,
};

VkDebugUtilsMessengerEXT debug_messenger;

#endif

// TODO: MID_PRIO Improve state machine to support swapchain recreating.

// Private global variables.
static Graphics_state_machine graphics_sm = GRAPHICS_SM_PRE_INIT;
static Graphics_error graphics_last_error = GRAPHICS_OK;
static Graphics_notification graphics_notification = {};
static VkInstance graphics_vulkan_instance = NULL;
static VkSurfaceKHR graphics_surface = NULL;
static VkSwapchainKHR graphics_swapchain = NULL;
static VkPhysicalDevice graphics_physical_device = NULL;
static VkDevice graphics_logical_device = NULL;


// Private function declarations.
static Graphics_error graphics_create_vulkan_instance();
static VkPhysicalDevice graphics_pick_device(
                                            const char* desired_extensions[],
                                            uint32_t desired_extensions_count,
                                            VkPhysicalDeviceFeatures* desired_features,
                                            VkQueueFlags desired_queue_features);
static VkSurfaceKHR graphics_create_surface(); 
static Graphics_queue_properties graphics_pick_queue_family(VkPhysicalDevice device, VkQueueFlags desired_queue_features);

// Private state machine functions.
static inline void graphics_sm_state_transition(Graphics_state_machine state);
static inline void graphics_sm_state_transition_error(Graphics_error error);
static Graphics_error graphics_sm_pre_init();
static Graphics_error graphics_sm_init();
static Graphics_error graphics_sm_swapchain_creation();

// Module's main function.
Graphics_error graphics_state_machine_loop()
{
    Graphics_error state_error;

    switch(graphics_sm)
    {
    case GRAPHICS_SM_PRE_INIT:
        state_error = graphics_sm_pre_init();

        if(state_error == GRAPHICS_OK)  graphics_sm_state_transition(GRAPHICS_SM_WAIT_FOR_WINDOW_CREATION);
        else                            graphics_sm_state_transition_error(state_error);

        break;

    case GRAPHICS_SM_WAIT_FOR_WINDOW_CREATION:
        if(graphics_notification.window_created)
        {
            graphics_notification.window_created = 0;
            graphics_sm_state_transition(GRAPHICS_SM_INIT);
        }

        break;

    case GRAPHICS_SM_INIT:
        state_error = graphics_sm_init();

        if(state_error == GRAPHICS_OK)  graphics_sm_state_transition(GRAPHICS_SM_SWAPCHAIN_CREATION);
        else                            graphics_sm_state_transition_error(state_error);

        break;

    case GRAPHICS_SM_SWAPCHAIN_CREATION:
        state_error = graphics_sm_swapchain_creation();

        if(state_error == GRAPHICS_OK)  graphics_sm_state_transition(GRAPHICS_SM_RENDER);
        else                            graphics_sm_state_transition_error(state_error);

        break;

    case GRAPHICS_SM_ERROR:
        break;

    default:
        break;
    }

    return graphics_last_error;
}

inline Graphics_state_machine graphics_sm_get_state()
{
    return graphics_sm;
}

inline void graphics_sm_notify(Graphics_notification notification)
{
    graphics_notification.value |= notification.value;
}

static inline void graphics_sm_state_transition(Graphics_state_machine state)
{
    graphics_sm = state;
}

static inline void graphics_sm_state_transition_error(Graphics_error error)
{
    graphics_last_error = error;
    graphics_sm_state_transition(GRAPHICS_SM_ERROR);
}

// GRAPHICS_SM_PRE_INIT
static Graphics_error graphics_sm_pre_init()
{
    VkResult vk_result = VK_SUCCESS;

    if(volkInitialize() == VK_ERROR_INITIALIZATION_FAILED) return GRAPHICS_VULKAN_INITIALIZATION;

    Graphics_error result = graphics_create_vulkan_instance();
    if(result != GRAPHICS_OK) return result;

    volkLoadInstanceOnly(graphics_vulkan_instance);

#ifdef DEBUG
    vk_result = vkCreateDebugUtilsMessengerEXT(graphics_vulkan_instance, &debug_messenger_create_info, NULL, &debug_messenger);
    assert(vk_result == VK_SUCCESS);
#endif

    return GRAPHICS_OK;
}

static Graphics_error graphics_sm_init()
{
    VkResult vk_result = VK_SUCCESS;

    graphics_surface = graphics_create_surface();
    if(graphics_surface == NULL) return GRAPHICS_VULKAN_SURFACE_CREATE;
    
    const char* desired_extensions[] = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };
    uint32_t desired_extensions_count = sizeof(desired_extensions) / sizeof(char*);

    VkPhysicalDeviceFeatures desired_features = {};
    desired_features.geometryShader = VK_TRUE;
    desired_features.tessellationShader = VK_TRUE;

    VkQueueFlags desired_queue_properties = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

    graphics_physical_device = graphics_pick_device(desired_extensions, desired_extensions_count, &desired_features, desired_queue_properties);
    if(graphics_physical_device == NULL) return GRAPHICS_VULKAN_NO_USABLE_PHYSICAL_DEVICE;

#ifdef DEBUG
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(graphics_physical_device, &properties);
    
    g_printf("Chosen device's name: %s\n", properties.deviceName);
#endif

    Graphics_queue_properties queue_family_properties = graphics_pick_queue_family(graphics_physical_device, desired_queue_properties);
    if(queue_family_properties.index == UINT32_MAX) return GRAPHICS_VULKAN_NO_USABLE_QUEUE_FAMILY;

    float queue_priorities[2] = {1.0f, 0.5f};

    VkDeviceQueueCreateInfo queue_create_info = 
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = queue_family_properties.index,
        .queueCount = 2,
        .pQueuePriorities = queue_priorities
    };

    VkDeviceCreateInfo device_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = desired_extensions_count,
        .ppEnabledExtensionNames = desired_extensions,
        .pEnabledFeatures = &desired_features
    };

    vk_result = vkCreateDevice(graphics_physical_device, &device_create_info, NULL, &graphics_logical_device);
    if(vk_result != VK_SUCCESS) return GRAPHICS_VULKAN_LOGICAL_DEVICE_CREATE;

    volkLoadDevice(graphics_logical_device);

    return GRAPHICS_OK;
}

static Graphics_error graphics_sm_swapchain_creation()
{
    assert(graphics_surface != NULL);
    assert(graphics_physical_device != NULL);
    assert(graphics_logical_device != NULL);

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D image_size = {};
    VkSurfaceFormatKHR image_format = {};

    uint32_t presentation_mode_count = 0;

    VkResult vk_result = vkGetPhysicalDeviceSurfacePresentModesKHR(graphics_physical_device, graphics_surface, &presentation_mode_count, NULL);
    if(vk_result != VK_SUCCESS) return GRAPHICS_VULKAN_SWAPCHAIN_CREATE;
    VkPresentModeKHR* presetation_modes = (VkPresentModeKHR*)malloc(presentation_mode_count * sizeof(VkPresentModeKHR));

    vk_result = vkGetPhysicalDeviceSurfacePresentModesKHR(graphics_physical_device, graphics_surface, &presentation_mode_count, presetation_modes);
    if(vk_result != VK_SUCCESS)
    {
        free(presetation_modes);
        return GRAPHICS_VULKAN_SWAPCHAIN_CREATE;
    }

    for(uint32_t i = 0; i < presentation_mode_count; i++)
    {
        if(presetation_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    }

    free(presetation_modes);

    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    vk_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphics_physical_device, graphics_surface, &surface_capabilities);
    if(vk_result != VK_SUCCESS) return GRAPHICS_VULKAN_SWAPCHAIN_CREATE;

    if(surface_capabilities.currentExtent.width == 0xFFFFFFFF)
    {
        image_size.width = SETTINGS_WIDTH;
        image_size.height = SETTINGS_HEIGHT;
    }
    else
    {
        image_size = surface_capabilities.currentExtent;
    }

    if(image_size.width < surface_capabilities.minImageExtent.width) image_size.width = surface_capabilities.minImageExtent.width;
    if(image_size.width > surface_capabilities.maxImageExtent.width) image_size.width = surface_capabilities.maxImageExtent.width;

    if(image_size.height < surface_capabilities.minImageExtent.height) image_size.height = surface_capabilities.minImageExtent.height;
    if(image_size.height > surface_capabilities.maxImageExtent.height) image_size.height = surface_capabilities.maxImageExtent.height;

    uint32_t formats_count = 0;

    vk_result = vkGetPhysicalDeviceSurfaceFormatsKHR(graphics_physical_device, graphics_surface, &formats_count, NULL);
    if(vk_result != VK_SUCCESS) return GRAPHICS_VULKAN_SWAPCHAIN_CREATE;
    VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)malloc(formats_count * sizeof(VkSurfaceFormatKHR));

    vk_result = vkGetPhysicalDeviceSurfaceFormatsKHR(graphics_physical_device, graphics_surface, &formats_count, formats);
    if(vk_result != VK_SUCCESS) 
    {
        free(formats);
        if(vk_result != VK_SUCCESS) return GRAPHICS_VULKAN_SWAPCHAIN_CREATE;
    }

    if(formats_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        image_format.format = VK_FORMAT_B8G8R8A8_SRGB;
        image_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }
    else
    {
        image_format = formats[0];
        for(uint32_t i = 0; i < formats_count; i++)
        {
            if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB || formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
            {
                image_format = formats[i];
            }
        }
    }

    free(formats);

    g_printf("Swapchain image size: %dx%d, Images count: %d, Presentation mode: %s.\n", 
        image_size.width,
        image_size.height,
        surface_capabilities.minImageCount,
        (present_mode == VK_PRESENT_MODE_MAILBOX_KHR ? "Mailbox" : "FIFO"));

    VkSwapchainKHR old_swapchain = NULL;
    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = graphics_surface,
        .minImageCount = surface_capabilities.minImageCount,
        .imageFormat = image_format.format,
        .imageColorSpace = image_format.colorSpace,
        .imageExtent = image_size,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain
    };

    vk_result = vkCreateSwapchainKHR(graphics_logical_device, &swapchain_info, NULL, &graphics_swapchain);
    if(vk_result != VK_SUCCESS) return GRAPHICS_VULKAN_SWAPCHAIN_CREATE;

    if(old_swapchain != NULL) vkDestroySwapchainKHR(graphics_logical_device, old_swapchain, NULL);

    return GRAPHICS_OK;
}

void graphics_cleanup()
{
    if(graphics_swapchain != NULL) vkDestroySwapchainKHR(graphics_logical_device, graphics_swapchain, NULL);
    if(graphics_logical_device != NULL) vkDestroyDevice(graphics_logical_device, NULL);
    if(graphics_surface != NULL) vkDestroySurfaceKHR(graphics_vulkan_instance, graphics_surface, NULL);

#ifdef DEBUG
    if(graphics_vulkan_instance != NULL) vkDestroyDebugUtilsMessengerEXT(graphics_vulkan_instance, debug_messenger, NULL);
#endif

    if(graphics_vulkan_instance != NULL) vkDestroyInstance(graphics_vulkan_instance, NULL);

    volkFinalize();
}

static Graphics_error graphics_create_vulkan_instance()
{
#ifdef DEBUG
    const char* desired_layers[] = 
    {
        "VK_LAYER_KHRONOS_validation",
    };
    uint32_t desired_layers_count = sizeof(desired_layers) / sizeof(char*);
#endif

    // TODO: MID_PRIO Enhance in the future the extension selection on a different platforms. 
    const char* desired_extensions[] = 
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
        "VK_KHR_win32_surface",
#elif defined(__APPLE__)
        "VK_EXT_metal_surface", // Other possible extension: VK_MVK_macos_surface
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
        "VK_KHR_wayland_surface",
#else
        "VK_KHR_xlib_surface", // TODO: MID_PRIO Change from Xlib to Xcb after GLFW 3.5 release.
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

    // TODO: LOW_PRIO Improve this loop
    uint32_t found_extension_count = 0;
    for(uint32_t i = 0; i < desired_extensions_count; i++)
    {
        for(uint32_t j = 0; j < extension_count; j++)
        {
            if(strcmp(desired_extensions[i], extensions[j].extensionName) == 0)
            {
                ++found_extension_count;
            }
        }
    }
    free(extensions);

    if(found_extension_count != desired_extensions_count) return GRAPHICS_VULKAN_NO_INSTANCE_EXTENSIONS;

    VkApplicationInfo app_info = 
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = APPLICATION_NAME,
        .applicationVersion = APPLICATION_VERSION,
        .pEngineName = APPLICATION_NAME,
        .engineVersion = APPLICATION_VERSION,
        .apiVersion = VK_API_VERSION_1_1
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

    VkResult result = vkCreateInstance(&instance_info, NULL, &graphics_vulkan_instance);

    if(result != VK_SUCCESS || graphics_vulkan_instance == NULL) return GRAPHICS_VULKAN_INSTANCE_CREATE;

    return GRAPHICS_OK;
}

static VkSurfaceKHR graphics_create_surface()
{
    VkSurfaceKHR surface = NULL;
    VkResult result = VK_SUCCESS;
    Window_creation_info window_info = {};

    window_get_creation_info(&window_info);

#ifdef _WIN32

    VkWin32SurfaceCreateInfoKHR surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hwnd = window_info.hwnd,
        .hinstance = window_info.hinstance
    };

    result = vkCreateWin32SurfaceKHR(graphics_vulkan_instance, &surface_create_info, NULL, &surface);

#elif defined(__APPLE__)

    VkMacOSSurfaceCreateInfoMVK  surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
        .pNext = NULL,
        .flags = 0,
        .pView = window_info.pView
    };

    result = vkCreateMacOSSurfaceMVK(graphics_vulkan_instance, &surface_create_info, NULL, &surface);

#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)


    VkWaylandSurfaceCreateInfoKHR  surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .display = window_info.display,
        .surface = window_info.surface
    };

    result = vkCreateWaylandSurfaceKHR(graphics_vulkan_instance, &surface_create_info, NULL, &surface);

#else 

// TODO: MID_PRIO Change from Xlib to Xcb after GLFW 3.5 release.
    VkWaylandSurfaceCreateInfoKHR  surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .dpy = window_info.dpy,
        .window = window_info.window
    };

    result = vkCreateXlibSurfaceKHR(graphics_vulkan_instance, &surface_create_info, NULL, &surface);

#endif

    if(result != VK_SUCCESS) return NULL;

    return surface;
}

static bool graphics_is_device_usable(
                                        VkPhysicalDevice device,
                                        const char* desired_extensions[],
                                        uint32_t desired_extensions_count,
                                        VkPhysicalDeviceFeatures* desired_features,
                                        VkQueueFlags desired_queue_features
                                     )
{
    assert(graphics_surface != NULL);

    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);

    VkExtensionProperties* extensions = (VkExtensionProperties*)malloc(extension_count * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, extensions);

    // TODO: LOW_PRIO Improve this loop
    uint32_t found_extension_count = 0;
    for(uint32_t i = 0; i < desired_extensions_count; i++)
    {
        for(uint32_t j = 0; j < extension_count; j++)
        {
            if(strcmp(desired_extensions[i], extensions[j].extensionName) == 0)
            {
                ++found_extension_count;
            }
        }
    }

    free(extensions);

    bool extensions_present = found_extension_count == desired_extensions_count;
    bool features_present = true;

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    for(uint32_t i = 0; i < sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32); i++)
    {
        VkBool32* feature_field = ((VkBool32*)&features) + i;
        VkBool32* desired_feature_field = ((VkBool32*)desired_features) + i;

        if((*desired_feature_field) == VK_FALSE) continue;
        if((*feature_field) != VK_TRUE) 
        {
            features_present = false;
            break;
        }
    }

    uint32_t queue_families_count = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, NULL);

    VkQueueFamilyProperties* queue_families = (VkQueueFamilyProperties*)malloc(queue_families_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families);

    bool queue_features_present = false, presentation_feature_present = false; 
    for(uint32_t i = 0; i < queue_families_count; i++)
    {
        if(queue_families[i].queueCount < 1) continue;

        if((queue_families[i].queueFlags & desired_queue_features) == desired_queue_features)
        {
            queue_features_present = true;
        }

        VkBool32 presentation_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, graphics_surface, &presentation_supported);

        if(presentation_supported == VK_TRUE) presentation_feature_present = true;

        if(queue_features_present && presentation_feature_present) break;
    }

    free(queue_families);

    return extensions_present && features_present && queue_features_present && presentation_feature_present;
}

static VkPhysicalDevice graphics_pick_device(
                                        const char* desired_extensions[],
                                        uint32_t desired_extensions_count,
                                        VkPhysicalDeviceFeatures* desired_features,
                                        VkQueueFlags desired_queue_features)
{
    VkPhysicalDevice picked_device = NULL;
    
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(graphics_vulkan_instance, &device_count, NULL);

    if(device_count == 0) return NULL;

    VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(graphics_vulkan_instance, &device_count, devices);

    uint16_t* device_usabilty = (uint16_t*)calloc(device_count, sizeof(uint16_t));
    uint16_t highest_usability = 0;
    uint32_t device_index_highest_usability = device_count;

    for(uint32_t i = 0; i < device_count; i++)
    {
        if(!graphics_is_device_usable(devices[i], desired_extensions, desired_extensions_count, desired_features, desired_queue_features)) continue;

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(devices[i], &features);

        switch(properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: device_usabilty[i] += 100; break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   device_usabilty[i] += 200; break;
            default: break;
        }

        for(uint32_t j = 0; j < sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32); j++)
        {
            VkBool32* field = ((VkBool32*)&features) + j;
            if(*field == VK_TRUE) device_usabilty[i]++;
        }

        if(highest_usability < device_usabilty[i]) 
        {
            highest_usability = device_usabilty[i];
            device_index_highest_usability = i;
        }
    }

    if(device_index_highest_usability != device_count) picked_device = devices[device_index_highest_usability];
    
    free(devices);
    free(device_usabilty);

    return picked_device;
}

static Graphics_queue_properties graphics_pick_queue_family(VkPhysicalDevice device, VkQueueFlags desired_queue_features)
{
    assert(graphics_surface != NULL);

    uint32_t queue_families_count = 0;
    Graphics_queue_properties queue_family_properties = {.index = UINT32_MAX, .properties = {}};

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, NULL);

    VkQueueFamilyProperties* queue_families = (VkQueueFamilyProperties*)malloc(queue_families_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families);

    for(uint32_t i = 0; i < queue_families_count; i++)
    {
        if(queue_families[i].queueCount < 1) continue;

        VkBool32 presentation_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, graphics_surface, &presentation_supported);

        if(((queue_families[i].queueFlags & desired_queue_features) == desired_queue_features) && presentation_supported == VK_TRUE)
        {
            queue_family_properties.index = i;
            queue_family_properties.properties = queue_families[i];
            break;
        }
    }

    free(queue_families);

    return queue_family_properties;
}