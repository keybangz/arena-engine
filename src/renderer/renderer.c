#include "renderer.h"
#include "arena/alloc/arena_allocator.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// ============================================================================
// Constants
// ============================================================================

#define MAX_FRAMES_IN_FLIGHT 2

static const char* VALIDATION_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation"
};
static const uint32_t VALIDATION_LAYER_COUNT = 1;

static const char* DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static const uint32_t DEVICE_EXTENSION_COUNT = 1;

// ============================================================================
// Renderer Structure
// ============================================================================

struct Renderer {
    GLFWwindow* window;
    Arena* arena;
    RendererConfig config;

    // Vulkan core
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;

    // Queues
    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_family;
    uint32_t present_family;

    // Swapchain
    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    uint32_t swapchain_image_count;
    VkImage* swapchain_images;
    VkImageView* swapchain_image_views;

    // Render pass
    VkRenderPass render_pass;
    VkFramebuffer* framebuffers;

    // Depth buffer
    VkImage depth_image;
    VkDeviceMemory depth_memory;
    VkImageView depth_view;
    VkFormat depth_format;

    // Pipeline (triangle)
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    // Pipeline (quad with push constants)
    VkPipelineLayout quad_pipeline_layout;
    VkPipeline quad_pipeline;

    // Command buffers
    VkCommandPool command_pool;
    VkCommandBuffer* command_buffers;

    // Synchronization
    VkSemaphore* image_available_semaphores;
    VkSemaphore* render_finished_semaphores;
    VkFence* in_flight_fences;

    // State
    uint32_t current_frame;
    uint64_t frame_count;
    bool framebuffer_resized;
    bool is_valid;
};

// ============================================================================
// Debug Callback
// ============================================================================

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    (void)type; (void)user_data;

    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        fprintf(stderr, "Vulkan: %s\n", callback_data->pMessage);
    }
    return VK_FALSE;
}

// ============================================================================
// Helper Functions
// ============================================================================

static bool check_validation_layer_support(void) {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    VkLayerProperties* layers = malloc(sizeof(VkLayerProperties) * layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layers);

    for (uint32_t i = 0; i < VALIDATION_LAYER_COUNT; i++) {
        bool found = false;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (strcmp(VALIDATION_LAYERS[i], layers[j].layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            free(layers);
            return false;
        }
    }

    free(layers);
    return true;
}


// ============================================================================
// Instance Creation
// ============================================================================

static bool create_instance(Renderer* r) {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Arena Engine",
        .applicationVersion = VK_MAKE_VERSION(0, 2, 0),
        .pEngineName = "Arena",
        .engineVersion = VK_MAKE_VERSION(0, 2, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    // Get required extensions from GLFW
    uint32_t glfw_ext_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    // Add debug extension if validation enabled
    uint32_t ext_count = glfw_ext_count;
    const char** extensions = malloc(sizeof(char*) * (glfw_ext_count + 1));
    memcpy(extensions, glfw_extensions, sizeof(char*) * glfw_ext_count);

    if (r->config.enable_validation) {
        extensions[ext_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = ext_count,
        .ppEnabledExtensionNames = extensions
    };

    if (r->config.enable_validation && check_validation_layer_support()) {
        create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
        printf("Vulkan validation layers enabled\n");
    }

    VkResult result = vkCreateInstance(&create_info, NULL, &r->instance);
    free(extensions);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance: %d\n", result);
        return false;
    }

    printf("Vulkan instance created\n");
    return true;
}

static bool setup_debug_messenger(Renderer* r) {
    if (!r->config.enable_validation) return true;

    VkDebugUtilsMessengerCreateInfoEXT create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback
    };

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            r->instance, "vkCreateDebugUtilsMessengerEXT");

    if (func && func(r->instance, &create_info, NULL, &r->debug_messenger) == VK_SUCCESS) {
        return true;
    }

    fprintf(stderr, "Failed to setup debug messenger\n");
    return false;
}

// ============================================================================
// Physical Device Selection
// ============================================================================

static bool find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface,
                                 uint32_t* graphics, uint32_t* present) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);

    VkQueueFamilyProperties* props = malloc(sizeof(VkQueueFamilyProperties) * count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props);

    bool found_graphics = false, found_present = false;

    for (uint32_t i = 0; i < count; i++) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            *graphics = i;
            found_graphics = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        if (present_support) {
            *present = i;
            found_present = true;
        }

        if (found_graphics && found_present) break;
    }

    free(props);
    return found_graphics && found_present;
}

static bool check_device_extension_support(VkPhysicalDevice device) {
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &count, NULL);

    VkExtensionProperties* props = malloc(sizeof(VkExtensionProperties) * count);
    vkEnumerateDeviceExtensionProperties(device, NULL, &count, props);

    for (uint32_t i = 0; i < DEVICE_EXTENSION_COUNT; i++) {
        bool found = false;
        for (uint32_t j = 0; j < count; j++) {
            if (strcmp(DEVICE_EXTENSIONS[i], props[j].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            free(props);
            return false;
        }
    }

    free(props);
    return true;
}

static bool pick_physical_device(Renderer* r) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(r->instance, &device_count, NULL);

    if (device_count == 0) {
        fprintf(stderr, "No Vulkan-capable GPUs found\n");
        return false;
    }

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(r->instance, &device_count, devices);

    for (uint32_t i = 0; i < device_count; i++) {
        uint32_t graphics, present;
        if (find_queue_families(devices[i], r->surface, &graphics, &present) &&
            check_device_extension_support(devices[i])) {
            r->physical_device = devices[i];
            r->graphics_family = graphics;
            r->present_family = present;

            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(devices[i], &props);
            printf("Selected GPU: %s\n", props.deviceName);

            free(devices);
            return true;
        }
    }

    free(devices);
    fprintf(stderr, "No suitable GPU found\n");
    return false;
}


// ============================================================================
// Logical Device Creation
// ============================================================================

static bool create_logical_device(Renderer* r) {
    float queue_priority = 1.0f;

    // Create queue create infos
    uint32_t unique_families[2] = {r->graphics_family, r->present_family};
    uint32_t unique_count = (r->graphics_family == r->present_family) ? 1 : 2;

    VkDeviceQueueCreateInfo queue_infos[2];
    for (uint32_t i = 0; i < unique_count; i++) {
        queue_infos[i] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = unique_families[i],
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };
    }

    VkPhysicalDeviceFeatures features = {0};

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = unique_count,
        .pQueueCreateInfos = queue_infos,
        .pEnabledFeatures = &features,
        .enabledExtensionCount = DEVICE_EXTENSION_COUNT,
        .ppEnabledExtensionNames = DEVICE_EXTENSIONS
    };

    if (vkCreateDevice(r->physical_device, &create_info, NULL, &r->device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device\n");
        return false;
    }

    vkGetDeviceQueue(r->device, r->graphics_family, 0, &r->graphics_queue);
    vkGetDeviceQueue(r->device, r->present_family, 0, &r->present_queue);

    printf("Vulkan logical device created\n");
    return true;
}

// ============================================================================
// Swapchain
// ============================================================================

static VkSurfaceFormatKHR choose_surface_format(VkSurfaceFormatKHR* formats, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[i];
        }
    }
    return formats[0];
}

static VkPresentModeKHR choose_present_mode(VkPresentModeKHR* modes, uint32_t count, bool vsync) {
    if (!vsync) {
        for (uint32_t i = 0; i < count; i++) {
            if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) return modes[i];
        }
        for (uint32_t i = 0; i < count; i++) {
            if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) return modes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_extent(VkSurfaceCapabilitiesKHR* caps, GLFWwindow* window) {
    if (caps->currentExtent.width != UINT32_MAX) {
        return caps->currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent = {(uint32_t)width, (uint32_t)height};
    if (extent.width < caps->minImageExtent.width) extent.width = caps->minImageExtent.width;
    if (extent.width > caps->maxImageExtent.width) extent.width = caps->maxImageExtent.width;
    if (extent.height < caps->minImageExtent.height) extent.height = caps->minImageExtent.height;
    if (extent.height > caps->maxImageExtent.height) extent.height = caps->maxImageExtent.height;

    return extent;
}

static bool create_swapchain(Renderer* r) {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r->physical_device, r->surface, &caps);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(r->physical_device, r->surface, &format_count, NULL);
    VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(r->physical_device, r->surface, &format_count, formats);

    uint32_t mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(r->physical_device, r->surface, &mode_count, NULL);
    VkPresentModeKHR* modes = malloc(sizeof(VkPresentModeKHR) * mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(r->physical_device, r->surface, &mode_count, modes);

    VkSurfaceFormatKHR surface_format = choose_surface_format(formats, format_count);
    VkPresentModeKHR present_mode = choose_present_mode(modes, mode_count, r->config.enable_vsync);
    VkExtent2D extent = choose_extent(&caps, r->window);

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = r->surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    uint32_t queue_families[] = {r->graphics_family, r->present_family};
    if (r->graphics_family != r->present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_families;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    free(formats);
    free(modes);

    if (vkCreateSwapchainKHR(r->device, &create_info, NULL, &r->swapchain) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }

    r->swapchain_format = surface_format.format;
    r->swapchain_extent = extent;

    // Get swapchain images
    vkGetSwapchainImagesKHR(r->device, r->swapchain, &r->swapchain_image_count, NULL);
    r->swapchain_images = malloc(sizeof(VkImage) * r->swapchain_image_count);
    vkGetSwapchainImagesKHR(r->device, r->swapchain, &r->swapchain_image_count, r->swapchain_images);

    // Create image views
    r->swapchain_image_views = malloc(sizeof(VkImageView) * r->swapchain_image_count);
    for (uint32_t i = 0; i < r->swapchain_image_count; i++) {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = r->swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = r->swapchain_format,
            .components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                           VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        };

        if (vkCreateImageView(r->device, &view_info, NULL, &r->swapchain_image_views[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create image view\n");
            return false;
        }
    }

    printf("Swapchain created: %dx%d, %d images\n",
           extent.width, extent.height, r->swapchain_image_count);
    return true;
}


// ============================================================================
// Memory Helpers
// ============================================================================

static uint32_t find_memory_type(VkPhysicalDevice pd, uint32_t filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(pd, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & props) == props) return i;
    }
    return 0;
}

// ============================================================================
// Depth Buffer
// ============================================================================

static VkFormat find_depth_format(VkPhysicalDevice pd) {
    VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    for (int i = 0; i < 3; i++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(pd, candidates[i], &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return candidates[i];
        }
    }
    return VK_FORMAT_D32_SFLOAT;
}

static bool create_depth_buffer(Renderer* r) {
    r->depth_format = find_depth_format(r->physical_device);

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = r->depth_format,
        .extent = {r->swapchain_extent.width, r->swapchain_extent.height, 1},
        .mipLevels = 1, .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    if (vkCreateImage(r->device, &image_info, NULL, &r->depth_image) != VK_SUCCESS) return false;

    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements(r->device, r->depth_image, &reqs);
    VkMemoryAllocateInfo alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = reqs.size,
        .memoryTypeIndex = find_memory_type(r->physical_device, reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    if (vkAllocateMemory(r->device, &alloc, NULL, &r->depth_memory) != VK_SUCCESS) return false;
    vkBindImageMemory(r->device, r->depth_image, r->depth_memory, 0);

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = r->depth_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = r->depth_format,
        .subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
    };
    if (vkCreateImageView(r->device, &view_info, NULL, &r->depth_view) != VK_SUCCESS) return false;

    printf("Depth buffer created (format: %d)\n", r->depth_format);
    return true;
}

static void destroy_depth_buffer(Renderer* r) {
    if (r->depth_view) vkDestroyImageView(r->device, r->depth_view, NULL);
    if (r->depth_memory) vkFreeMemory(r->device, r->depth_memory, NULL);
    if (r->depth_image) vkDestroyImage(r->device, r->depth_image, NULL);
    r->depth_view = VK_NULL_HANDLE;
    r->depth_memory = VK_NULL_HANDLE;
    r->depth_image = VK_NULL_HANDLE;
}

// ============================================================================
// Render Pass & Framebuffers
// ============================================================================

static bool create_render_pass(Renderer* r) {
    VkAttachmentDescription attachments[] = {
        // Color attachment
        {
            .format = r->swapchain_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },
        // Depth attachment
        {
            .format = r->depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };

    VkAttachmentReference color_ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_ref = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_ref,
        .pDepthStencilAttachment = &depth_ref
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    if (vkCreateRenderPass(r->device, &create_info, NULL, &r->render_pass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        return false;
    }

    printf("Render pass created\n");
    return true;
}

static bool create_framebuffers(Renderer* r) {
    r->framebuffers = malloc(sizeof(VkFramebuffer) * r->swapchain_image_count);

    for (uint32_t i = 0; i < r->swapchain_image_count; i++) {
        VkImageView attachments[] = {r->swapchain_image_views[i], r->depth_view};

        VkFramebufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = r->render_pass,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .width = r->swapchain_extent.width,
            .height = r->swapchain_extent.height,
            .layers = 1
        };

        if (vkCreateFramebuffer(r->device, &create_info, NULL, &r->framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer\n");
            return false;
        }
    }

    printf("Framebuffers created\n");
    return true;
}

// ============================================================================
// Shader Loading
// ============================================================================

static char* read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = malloc(size);
    if (!data) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(data, 1, size, f);
    fclose(f);

    if (read != size) {
        free(data);
        return NULL;
    }

    *out_size = size;
    return data;
}

static VkShaderModule create_shader_module(VkDevice device, const char* code, size_t size) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (const uint32_t*)code
    };

    VkShaderModule module;
    if (vkCreateShaderModule(device, &create_info, NULL, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

// ============================================================================
// Graphics Pipeline
// ============================================================================

static bool create_graphics_pipeline(Renderer* r) {
    // Load shader SPIR-V - try multiple paths
    size_t vert_size, frag_size;
    char* vert_code = read_file("shaders/triangle.vert.spv", &vert_size);
    if (!vert_code) vert_code = read_file("src/renderer/shaders/triangle.vert.spv", &vert_size);

    char* frag_code = read_file("shaders/triangle.frag.spv", &frag_size);
    if (!frag_code) frag_code = read_file("src/renderer/shaders/triangle.frag.spv", &frag_size);

    if (!vert_code || !frag_code) {
        fprintf(stderr, "Failed to load shader files\n");
        free(vert_code);
        free(frag_code);
        return false;
    }

    VkShaderModule vert_module = create_shader_module(r->device, vert_code, vert_size);
    VkShaderModule frag_module = create_shader_module(r->device, frag_code, frag_size);
    free(vert_code);
    free(frag_code);

    if (!vert_module || !frag_module) {
        fprintf(stderr, "Failed to create shader modules\n");
        if (vert_module) vkDestroyShaderModule(r->device, vert_module, NULL);
        if (frag_module) vkDestroyShaderModule(r->device, frag_module, NULL);
        return false;
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_module,
            .pName = "main"
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_module,
            .pName = "main"
        }
    };

    // Vertex input (empty - hardcoded in shader)
    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Dynamic state (viewport and scissor)
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_states
    };

    // Viewport state
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    // Depth stencil (disabled for 2D, but required since render pass has depth)
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    // Color blending
    VkPipelineColorBlendAttachmentState blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
    };
    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment
    };

    // Pipeline layout (empty for now)
    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };
    if (vkCreatePipelineLayout(r->device, &layout_info, NULL, &r->pipeline_layout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout\n");
        vkDestroyShaderModule(r->device, vert_module, NULL);
        vkDestroyShaderModule(r->device, frag_module, NULL);
        return false;
    }

    // Create pipeline
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = r->pipeline_layout,
        .renderPass = r->render_pass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(
        r->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &r->graphics_pipeline);

    vkDestroyShaderModule(r->device, vert_module, NULL);
    vkDestroyShaderModule(r->device, frag_module, NULL);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline\n");
        return false;
    }

    printf("Graphics pipeline created\n");
    return true;
}

// Push constant structure for quad rendering
typedef struct QuadPushConstants {
    float rect[4];       // x, y, width, height
    float color[4];      // r, g, b, a
    float screenSize[2]; // width, height
} QuadPushConstants;

static bool create_quad_pipeline(Renderer* r) {
    // Load quad shaders
    size_t vert_size, frag_size;
    char* vert_code = read_file("shaders/quad.vert.spv", &vert_size);
    if (!vert_code) vert_code = read_file("src/renderer/shaders/quad.vert.spv", &vert_size);

    char* frag_code = read_file("shaders/quad.frag.spv", &frag_size);
    if (!frag_code) frag_code = read_file("src/renderer/shaders/quad.frag.spv", &frag_size);

    if (!vert_code || !frag_code) {
        fprintf(stderr, "Failed to load quad shader files\n");
        free(vert_code);
        free(frag_code);
        return false;
    }

    VkShaderModule vert_module = create_shader_module(r->device, vert_code, vert_size);
    VkShaderModule frag_module = create_shader_module(r->device, frag_code, frag_size);
    free(vert_code);
    free(frag_code);

    if (!vert_module || !frag_module) {
        fprintf(stderr, "Failed to create quad shader modules\n");
        if (vert_module) vkDestroyShaderModule(r->device, vert_module, NULL);
        if (frag_module) vkDestroyShaderModule(r->device, frag_module, NULL);
        return false;
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_module,
            .pName = "main"
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_module,
            .pName = "main"
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_states
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_NONE,  // No culling for 2D
        .frontFace = VK_FRONT_FACE_CLOCKWISE
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    // Depth stencil (disabled for 2D quads)
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE
    };

    // Enable alpha blending
    VkPipelineColorBlendAttachmentState blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };
    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment
    };

    // Push constant range
    VkPushConstantRange push_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(QuadPushConstants)
    };

    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range
    };

    if (vkCreatePipelineLayout(r->device, &layout_info, NULL, &r->quad_pipeline_layout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create quad pipeline layout\n");
        vkDestroyShaderModule(r->device, vert_module, NULL);
        vkDestroyShaderModule(r->device, frag_module, NULL);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = r->quad_pipeline_layout,
        .renderPass = r->render_pass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(
        r->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &r->quad_pipeline);

    vkDestroyShaderModule(r->device, vert_module, NULL);
    vkDestroyShaderModule(r->device, frag_module, NULL);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create quad pipeline\n");
        return false;
    }

    printf("Quad pipeline created\n");
    return true;
}

// ============================================================================
// Command Buffers & Sync
// ============================================================================

static bool create_command_pool(Renderer* r) {
    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = r->graphics_family
    };

    if (vkCreateCommandPool(r->device, &create_info, NULL, &r->command_pool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool\n");
        return false;
    }

    return true;
}

static bool create_command_buffers(Renderer* r) {
    r->command_buffers = malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = r->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    if (vkAllocateCommandBuffers(r->device, &alloc_info, r->command_buffers) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffers\n");
        return false;
    }

    printf("Command buffers allocated\n");
    return true;
}

static bool create_sync_objects(Renderer* r) {
    r->image_available_semaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    r->render_finished_semaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    r->in_flight_fences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo sem_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(r->device, &sem_info, NULL, &r->image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(r->device, &sem_info, NULL, &r->render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(r->device, &fence_info, NULL, &r->in_flight_fences[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create sync objects\n");
            return false;
        }
    }

    printf("Sync objects created\n");
    return true;
}


// ============================================================================
// Swapchain Recreation
// ============================================================================

static void cleanup_swapchain(Renderer* r) {
    for (uint32_t i = 0; i < r->swapchain_image_count; i++) {
        vkDestroyFramebuffer(r->device, r->framebuffers[i], NULL);
        vkDestroyImageView(r->device, r->swapchain_image_views[i], NULL);
    }
    free(r->framebuffers);
    free(r->swapchain_image_views);
    free(r->swapchain_images);
    vkDestroySwapchainKHR(r->device, r->swapchain, NULL);
}

static bool recreate_swapchain(Renderer* r) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(r->window, &width, &height);

    // Use non-blocking poll instead of blocking wait to avoid freezing
    // when window is minimized or hidden on some window managers
    int max_attempts = 100;  // Prevent infinite loop
    while ((width == 0 || height == 0) && max_attempts-- > 0) {
        glfwGetFramebufferSize(r->window, &width, &height);
        glfwPollEvents();
        if (width == 0 || height == 0) {
            // Small sleep to avoid busy-waiting
            struct timespec ts = {0, 10000000};  // 10ms
            nanosleep(&ts, NULL);
        }
    }

    if (width == 0 || height == 0) {
        fprintf(stderr, "Warning: Window has zero size, skipping swapchain recreation\n");
        return false;
    }

    vkDeviceWaitIdle(r->device);
    cleanup_swapchain(r);
    destroy_depth_buffer(r);

    if (!create_swapchain(r)) return false;
    if (!create_depth_buffer(r)) return false;
    if (!create_framebuffers(r)) return false;

    r->framebuffer_resized = false;
    return true;
}

// ============================================================================
// Public API - Lifecycle
// ============================================================================

Renderer* renderer_create(GLFWwindow* window, Arena* arena) {
    return renderer_create_with_config(window, arena, renderer_config_default());
}

Renderer* renderer_create_with_config(GLFWwindow* window, Arena* arena, RendererConfig config) {
    Renderer* r = malloc(sizeof(Renderer));
    if (!r) return NULL;

    memset(r, 0, sizeof(Renderer));
    r->window = window;
    r->arena = arena;
    r->config = config;

    // Initialize Vulkan
    if (!create_instance(r)) goto fail;
    if (!setup_debug_messenger(r)) goto fail;

    // Create surface
    if (glfwCreateWindowSurface(r->instance, window, NULL, &r->surface) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create window surface\n");
        goto fail;
    }

    if (!pick_physical_device(r)) goto fail;
    if (!create_logical_device(r)) goto fail;
    if (!create_swapchain(r)) goto fail;
    if (!create_depth_buffer(r)) goto fail;
    if (!create_render_pass(r)) goto fail;
    if (!create_framebuffers(r)) goto fail;
    if (!create_graphics_pipeline(r)) goto fail;
    if (!create_quad_pipeline(r)) goto fail;
    if (!create_command_pool(r)) goto fail;
    if (!create_command_buffers(r)) goto fail;
    if (!create_sync_objects(r)) goto fail;

    r->is_valid = true;
    printf("Renderer initialized successfully\n");
    return r;

fail:
    renderer_destroy(r);
    return NULL;
}

void renderer_destroy(Renderer* r) {
    if (!r) return;

    if (r->device) {
        vkDeviceWaitIdle(r->device);

        // Sync objects
        if (r->in_flight_fences) {
            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroyFence(r->device, r->in_flight_fences[i], NULL);
                vkDestroySemaphore(r->device, r->render_finished_semaphores[i], NULL);
                vkDestroySemaphore(r->device, r->image_available_semaphores[i], NULL);
            }
            free(r->in_flight_fences);
            free(r->render_finished_semaphores);
            free(r->image_available_semaphores);
        }

        // Command buffers
        if (r->command_buffers) {
            free(r->command_buffers);
        }
        if (r->command_pool) {
            vkDestroyCommandPool(r->device, r->command_pool, NULL);
        }

        // Pipelines
        if (r->graphics_pipeline) {
            vkDestroyPipeline(r->device, r->graphics_pipeline, NULL);
        }
        if (r->pipeline_layout) {
            vkDestroyPipelineLayout(r->device, r->pipeline_layout, NULL);
        }
        if (r->quad_pipeline) {
            vkDestroyPipeline(r->device, r->quad_pipeline, NULL);
        }
        if (r->quad_pipeline_layout) {
            vkDestroyPipelineLayout(r->device, r->quad_pipeline_layout, NULL);
        }

        // Framebuffers and swapchain
        if (r->framebuffers) {
            for (uint32_t i = 0; i < r->swapchain_image_count; i++) {
                vkDestroyFramebuffer(r->device, r->framebuffers[i], NULL);
            }
            free(r->framebuffers);
        }

        if (r->render_pass) {
            vkDestroyRenderPass(r->device, r->render_pass, NULL);
        }

        // Depth buffer
        destroy_depth_buffer(r);

        if (r->swapchain_image_views) {
            for (uint32_t i = 0; i < r->swapchain_image_count; i++) {
                vkDestroyImageView(r->device, r->swapchain_image_views[i], NULL);
            }
            free(r->swapchain_image_views);
        }
        free(r->swapchain_images);

        if (r->swapchain) {
            vkDestroySwapchainKHR(r->device, r->swapchain, NULL);
        }

        vkDestroyDevice(r->device, NULL);
    }

    if (r->surface) {
        vkDestroySurfaceKHR(r->instance, r->surface, NULL);
    }

    if (r->debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                r->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) func(r->instance, r->debug_messenger, NULL);
    }

    if (r->instance) {
        vkDestroyInstance(r->instance, NULL);
    }

    free(r);
    printf("Renderer destroyed\n");
}


// ============================================================================
// Public API - Frame Operations
// ============================================================================

// Store image index for end_frame
static uint32_t g_current_image_index = 0;

bool renderer_begin_frame(Renderer* r) {
    if (!r || !r->is_valid) return false;

    // Wait for previous frame
    vkWaitForFences(r->device, 1, &r->in_flight_fences[r->current_frame], VK_TRUE, UINT64_MAX);

    // Acquire swapchain image
    VkResult result = vkAcquireNextImageKHR(
        r->device, r->swapchain, UINT64_MAX,
        r->image_available_semaphores[r->current_frame],
        VK_NULL_HANDLE, &g_current_image_index
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain(r);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Failed to acquire swapchain image\n");
        return false;
    }

    vkResetFences(r->device, 1, &r->in_flight_fences[r->current_frame]);

    // Reset and begin command buffer
    VkCommandBuffer cmd = r->command_buffers[r->current_frame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    vkBeginCommandBuffer(cmd, &begin_info);

    // Begin render pass - clear color and depth
    VkClearValue clear_values[] = {
        {.color = {{0.05f, 0.05f, 0.15f, 1.0f}}},
        {.depthStencil = {1.0f, 0}}
    };

    VkRenderPassBeginInfo pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = r->render_pass,
        .framebuffer = r->framebuffers[g_current_image_index],
        .renderArea = {{0, 0}, r->swapchain_extent},
        .clearValueCount = 2,
        .pClearValues = clear_values
    };

    vkCmdBeginRenderPass(cmd, &pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor
    VkViewport viewport = {
        .x = 0.0f, .y = 0.0f,
        .width = (float)r->swapchain_extent.width,
        .height = (float)r->swapchain_extent.height,
        .minDepth = 0.0f, .maxDepth = 1.0f
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {{0, 0}, r->swapchain_extent};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Draw calls happen between begin_frame and end_frame via renderer_draw_quad

    return true;
}

void renderer_draw_quad(Renderer* r,
                        float x, float y, float width, float height,
                        float cr, float cg, float cb, float ca) {
    if (!r || !r->is_valid) return;

    VkCommandBuffer cmd = r->command_buffers[r->current_frame];

    // Bind quad pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r->quad_pipeline);

    // Set push constants
    QuadPushConstants pc = {
        .rect = {x, y, width, height},
        .color = {cr, cg, cb, ca},
        .screenSize = {(float)r->swapchain_extent.width, (float)r->swapchain_extent.height}
    };

    vkCmdPushConstants(cmd, r->quad_pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

    // Draw 6 vertices (2 triangles = 1 quad)
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void renderer_end_frame(Renderer* r) {
    if (!r || !r->is_valid) return;

    VkCommandBuffer cmd = r->command_buffers[r->current_frame];

    // End render pass
    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        fprintf(stderr, "Failed to record command buffer\n");
        return;
    }

    // Submit
    VkSemaphore wait_semaphores[] = {r->image_available_semaphores[r->current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signal_semaphores[] = {r->render_finished_semaphores[r->current_frame]};

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores
    };

    if (vkQueueSubmit(r->graphics_queue, 1, &submit_info, r->in_flight_fences[r->current_frame]) != VK_SUCCESS) {
        fprintf(stderr, "Failed to submit draw command buffer\n");
        return;
    }

    // Present
    VkSwapchainKHR swapchains[] = {r->swapchain};
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &g_current_image_index
    };

    VkResult result = vkQueuePresentKHR(r->present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || r->framebuffer_resized) {
        recreate_swapchain(r);
    } else if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to present swapchain image\n");
    }

    r->current_frame = (r->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    r->frame_count++;
}

// ============================================================================
// Public API - State & Utilities
// ============================================================================

bool renderer_is_valid(const Renderer* r) {
    return r && r->is_valid;
}

void renderer_wait_idle(Renderer* r) {
    if (r && r->device) {
        vkDeviceWaitIdle(r->device);
    }
}

void renderer_on_resize(Renderer* r, int width, int height) {
    (void)width; (void)height;
    if (r) {
        r->framebuffer_resized = true;
    }
}

uint64_t renderer_get_frame_count(const Renderer* r) {
    return r ? r->frame_count : 0;
}

void* renderer_get_command_buffer(Renderer* r) {
    if (!r) return NULL;
    return r->command_buffers[r->current_frame];
}

void renderer_get_extent(Renderer* r, uint32_t* width, uint32_t* height) {
    if (!r) return;
    if (width) *width = r->swapchain_extent.width;
    if (height) *height = r->swapchain_extent.height;
}

void* renderer_get_device(Renderer* r) {
    return r ? (void*)r->device : NULL;
}

void* renderer_get_physical_device(Renderer* r) {
    return r ? (void*)r->physical_device : NULL;
}

void* renderer_get_render_pass(Renderer* r) {
    return r ? (void*)r->render_pass : NULL;
}

void* renderer_get_command_pool(Renderer* r) {
    return r ? (void*)r->command_pool : NULL;
}

void* renderer_get_graphics_queue(Renderer* r) {
    return r ? (void*)r->graphics_queue : NULL;
}