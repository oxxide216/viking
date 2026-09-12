#include <vulkan/vulkan.h>

#include "viking/viking.h"

#define APP_NAME    "Viking Application"
#define ENGINE_NAME "Viking Engine"

typedef struct {
  VkExtent2D          extent;
  VkSurfaceFormatKHR  format;
  VkSwapchainKHR      swap_chain;
  u32                 images_len;
  VkImage            *images;
  VkImageView        *image_views;
} WindowSizeDependantResources;

struct VikInstance {
  WinxWindow                   *window;
  VkInstance                    instance;
  VkSurfaceKHR                  surface;
  VkPhysicalDevice              physical_device;
  u32                           graphics_queue_family_index;
  u32                           present_queue_family_index;
  VkDevice                      device;
  VkQueue                       graphics_queue;
  VkQueue                       present_queue;
  WindowSizeDependantResources  resources;
  VkSemaphore                   image_available_semaphore;
  VkSemaphore                  *render_finished_semaphores;
  VkFence                       in_flight_fence;
  VkCommandPool                 temp_pool;
  VkDescriptorPool              descriptor_pool;
  u32                           image_index;
};

struct VikShader {
  VikInstance    *instance;
  VkShaderModule  vertex_module;
  VkShaderModule  fragment_module;
};

struct VikUBO {
  VikInstance    *instance;
  VkBuffer        buffer;
  VkDeviceMemory  buffer_memory;
  void           *data;
  u32             size;
};

struct VikPipeline {
  VikInstance           *instance;
  VkFramebuffer         *framebuffers;
  VkDescriptorSetLayout  descriptor_set_layout;
  VkDescriptorSet        descriptor_set;
  VkPipelineLayout       layout;
  VkRenderPass           render_pass;
  VkPipeline             pipeline;
  VikUBO                *ubo;
};

struct VikExecutor {
  VikInstance     *instance;
  VkCommandPool    pool;
  VkCommandBuffer  buffer;
};

struct VikMesh {
  VikInstance    *instance;
  VkBuffer        vertex_buffer;
  VkDeviceMemory  vertex_buffer_memory;
  VkBuffer        index_buffer;
  VkDeviceMemory  index_buffer_memory;
  u32             indices_len;
};

static _Thread_local char error_buffer[128];

static const char *vk_result_to_cstr(VkResult result)
{
  switch (result) {
  case VK_SUCCESS:                                            return "VK_SUCCESS";
  case VK_NOT_READY:                                          return "VK_NOT_READY";
  case VK_TIMEOUT:                                            return "VK_TIMEOUT";
  case VK_EVENT_SET:                                          return "VK_EVENT_SET";
  case VK_EVENT_RESET:                                        return "VK_EVENT_RESET";
  case VK_INCOMPLETE:                                         return "VK_INCOMPLETE";
  case VK_ERROR_OUT_OF_HOST_MEMORY:                           return "VK_ERROR_OUT_OF_HOST_MEMORY";
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:                         return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
  case VK_ERROR_INITIALIZATION_FAILED:                        return "VK_ERROR_INITIALIZATION_FAILED";
  case VK_ERROR_DEVICE_LOST:                                  return "VK_ERROR_DEVICE_LOST";
  case VK_ERROR_MEMORY_MAP_FAILED:                            return "VK_ERROR_MEMORY_MAP_FAILED";
  case VK_ERROR_LAYER_NOT_PRESENT:                            return "VK_ERROR_LAYER_NOT_PRESENT";
  case VK_ERROR_EXTENSION_NOT_PRESENT:                        return "VK_ERROR_EXTENSION_NOT_PRESENT";
  case VK_ERROR_FEATURE_NOT_PRESENT:                          return "VK_ERROR_FEATURE_NOT_PRESENT";
  case VK_ERROR_INCOMPATIBLE_DRIVER:                          return "VK_ERROR_INCOMPATIBLE_DRIVER";
  case VK_ERROR_TOO_MANY_OBJECTS:                             return "VK_ERROR_TOO_MANY_OBJECTS";
  case VK_ERROR_FORMAT_NOT_SUPPORTED:                         return "VK_ERROR_FORMAT_NOT_SUPPORTED";
  case VK_ERROR_FRAGMENTED_POOL:                              return "VK_ERROR_FRAGMENTED_POOL";
  case VK_ERROR_UNKNOWN:                                      return "VK_ERROR_UNKNOWN";
  case VK_ERROR_OUT_OF_POOL_MEMORY:                           return "VK_ERROR_OUT_OF_POOL_MEMORY";
  case VK_ERROR_INVALID_EXTERNAL_HANDLE:                      return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
  case VK_ERROR_FRAGMENTATION:                                return "VK_ERROR_FRAGMENTATION";
  case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:               return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
  case VK_PIPELINE_COMPILE_REQUIRED:                          return "VK_PIPELINE_COMPILE_REQUIRED";
  case VK_ERROR_SURFACE_LOST_KHR:                             return "VK_ERROR_SURFACE_LOST_KHR";
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                     return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
  case VK_SUBOPTIMAL_KHR:                                     return "VK_SUBOPTIMAL_KHR";
  case VK_ERROR_OUT_OF_DATE_KHR:                              return "VK_ERROR_OUT_OF_DATE_KHR";
  case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                     return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
  case VK_ERROR_VALIDATION_FAILED_EXT:                        return "VK_ERROR_VALIDATION_FAILED_EXT";
  case VK_ERROR_INVALID_SHADER_NV:                            return "VK_ERROR_INVALID_SHADER_NV";
#ifdef VK_ENABLE_BETA_EXTENSIONS
  case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:                return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
  case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:       return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
  case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:    return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
  case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:       return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
  case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:        return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
  case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:          return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
#endif
  case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
  case VK_ERROR_NOT_PERMITTED_KHR:                            return "VK_ERROR_NOT_PERMITTED_KHR";
  case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:          return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
  case VK_THREAD_IDLE_KHR:                                    return "VK_THREAD_IDLE_KHR";
  case VK_THREAD_DONE_KHR:                                    return "VK_THREAD_DONE_KHR";
  case VK_OPERATION_DEFERRED_KHR:                             return "VK_OPERATION_DEFERRED_KHR";
  case VK_OPERATION_NOT_DEFERRED_KHR:                         return "VK_OPERATION_NOT_DEFERRED_KHR";
  case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:                    return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
  case VK_RESULT_MAX_ENUM:                                    return "VK_RESULT_MAX_ENUM";
  default:                                                    return "??????";
  }
}

static bool make_window_size_dependant_resources_except_framebuffers(WindowSizeDependantResources *result,
                                                                     VkPhysicalDevice physical_device, VkDevice device,
                                                                                             VkSurfaceKHR surface,
                                                                                             u32 graphics_queue_family_index,
                                                                                             u32 present_queue_family_index,
                                                                                             u32 window_width, u32 window_height) {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

  result->extent = capabilities.currentExtent;
  if (result->extent.width == (u32) -1)
    result->extent = (VkExtent2D) { window_width, window_height };

  u32 formats_len;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_len, NULL);

  if (formats_len > 0) {
    VkSurfaceFormatKHR *formats = malloc(formats_len * sizeof(*formats));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_len, formats);

    result->format = formats[0];
    for (u32 i = 0; i < formats_len; ++i) {
      if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        result->format = formats[i];
        break;
      }
    }

    free(formats);
  }

  u32 present_modes_len;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_len, NULL);

  VkPresentModeKHR present_mode;

  if (present_modes_len > 0) {
    VkPresentModeKHR *present_modes = malloc(present_modes_len * sizeof(*present_modes));
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_len, present_modes);

    present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < present_modes_len; ++i) {
      if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        present_mode = present_modes[i];
        break;
      }
    }

    free(present_modes);
  }

  VkSwapchainCreateInfoKHR swap_chain_create_info = {0};
  swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swap_chain_create_info.surface = surface;
  swap_chain_create_info.minImageCount = capabilities.minImageCount + 1;
  swap_chain_create_info.imageFormat = result->format.format;
  swap_chain_create_info.imageColorSpace = result->format.colorSpace;
  swap_chain_create_info.imageExtent = result->extent;
  swap_chain_create_info.imageArrayLayers = 1;
  swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  if (graphics_queue_family_index == present_queue_family_index) {
    swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_chain_create_info.queueFamilyIndexCount = 0; // Optional
    swap_chain_create_info.pQueueFamilyIndices = NULL; // Optional
  } else {
    u32 queue_family_indices[] = {
      graphics_queue_family_index,
      present_queue_family_index,
    };
    swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swap_chain_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_family_indices);
    swap_chain_create_info.pQueueFamilyIndices = queue_family_indices;
  }

  swap_chain_create_info.preTransform = capabilities.currentTransform;
  swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swap_chain_create_info.presentMode = present_mode;
  swap_chain_create_info.clipped = VK_TRUE;
  swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

  VkResult swap_chain_result = vkCreateSwapchainKHR(device, &swap_chain_create_info, NULL, &result->swap_chain);
  if (swap_chain_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan swap chain: %s",
            vk_result_to_cstr(swap_chain_result));
    return false;
  }

  vkGetSwapchainImagesKHR(device, result->swap_chain, &result->images_len, NULL);

  result->images = malloc(result->images_len * sizeof(*result->images));
  vkGetSwapchainImagesKHR(device, result->swap_chain, &result->images_len, result->images);

  result->image_views = malloc(result->images_len * sizeof(*result->image_views));
  for (u32 i = 0; i < result->images_len; ++i) {
    VkImageViewCreateInfo image_view_create_info = {0};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = result->images[i];
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = result->format.format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    VkResult image_view_result = vkCreateImageView(device, &image_view_create_info, NULL, result->image_views + i);
    if (image_view_result != VK_SUCCESS) {
      sprintf(error_buffer, "Failed to create Vulkan image views: %s",
              vk_result_to_cstr(image_view_result));
      return false;
    }
  }

  return true;
}

static bool make_framebuffers(VkFramebuffer *framebuffers,
                              WindowSizeDependantResources *resources,
                              VkDevice device, VkRenderPass render_pass) {
  for (u32 i = 0; i < resources->images_len; ++i) {
    VkFramebufferCreateInfo framebuffer_create_info = {0};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = render_pass;
    framebuffer_create_info.attachmentCount = 1;
    framebuffer_create_info.pAttachments = resources->image_views + i;
    framebuffer_create_info.width = resources->extent.width;
    framebuffer_create_info.height = resources->extent.height;
    framebuffer_create_info.layers = 1;

    VkResult framebuffer_result = vkCreateFramebuffer(device, &framebuffer_create_info, NULL, framebuffers + i);
    if (framebuffer_result != VK_SUCCESS) {
      sprintf(error_buffer, "Failed to create Vulkan framebuffers: %s",
              vk_result_to_cstr(framebuffer_result));
      return false;
    }
  }

  return true;
}

static void delete_window_size_dependant_resources(WindowSizeDependantResources *resources,
                                                   VkDevice device) {
  for (u32 i = 0; i < resources->images_len; ++i)
    vkDestroyImageView(device, resources->image_views[i], NULL);

  free(resources->image_views);
  free(resources->images);

  vkDestroySwapchainKHR(device, resources->swap_chain, NULL);
}

VikInstance *vik_make_instance(WinxWindow *window) {
  VkApplicationInfo app_info = {0};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = APP_NAME;
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = ENGINE_NAME;
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  u32 extensions_len;
  const char * const *extensions = winx_get_vulkan_extensions(&extensions_len);

#ifndef VIK_NDEBUG
  const char *layers[] = { "VK_LAYER_KHRONOS_validation" };
#endif

  VkInstanceCreateInfo instance_create_info = {0};
  instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo = &app_info;
  instance_create_info.enabledExtensionCount = extensions_len;
  instance_create_info.ppEnabledExtensionNames = extensions;
#ifndef VIK_NDEBUG
  instance_create_info.enabledLayerCount = ARRAY_LEN(layers);
  instance_create_info.ppEnabledLayerNames = layers;
#else
  instance_create_info.enabledLayerCount = 0;
  instance_create_info.ppEnabledLayerNames = NULL;
#endif

  VkInstance instance;
  VkResult instance_result = vkCreateInstance(&instance_create_info, NULL, &instance);
  if (instance_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan instance: %s",
            vk_result_to_cstr(instance_result));
    return NULL;
  }

  VkSurfaceKHR surface = winx_create_vulkan_surface(window, instance, NULL);

  u32 physical_devices_len;
  vkEnumeratePhysicalDevices(instance, &physical_devices_len, NULL);

  if (physical_devices_len == 0) {
    sprintf(error_buffer, "No GPUs supporting Vulkan were found");
    return NULL;
  }

  VkPhysicalDevice *physical_devices = malloc(physical_devices_len * sizeof(*physical_devices));
  vkEnumeratePhysicalDevices(instance, &physical_devices_len, physical_devices);

  // TODO: allow library user to select a physical device
  VkPhysicalDevice physical_device;
  bool found_suitable_physical_device = false;
  u32 graphics_queue_family_index;
  u32 present_queue_family_index;

  for (u32 i = 0; i < 1; ++i) {
    for (u32 j = 0; j < physical_devices_len; ++j) {
      VkPhysicalDeviceProperties device_props;
      vkGetPhysicalDeviceProperties(physical_devices[j], &device_props);

      if (device_props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
          i == 0)
        continue;

      graphics_queue_family_index = (u32) -1;
      present_queue_family_index = (u32) -1;

      u32 queue_family_props_len;
      vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[j], &queue_family_props_len, NULL);

      VkQueueFamilyProperties *queue_family_props = malloc(queue_family_props_len * sizeof (*queue_family_props));
      vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[j], &queue_family_props_len, queue_family_props);

      for (u32 k = 0; k < queue_family_props_len; ++k) {
        if (queue_family_props[k].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          graphics_queue_family_index = k;
          if (present_queue_family_index != (u32) -1)
            break;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[j], k, surface, &present_support);
        if (present_support) {
          present_queue_family_index = k;
          if (graphics_queue_family_index != (u32) -1)
            break;
        }
      }

      if (queue_family_props)
        free(queue_family_props);

      u32 device_extension_props_len;
      vkEnumerateDeviceExtensionProperties(physical_devices[j], NULL, &device_extension_props_len, NULL);

      VkExtensionProperties *device_extension_props = malloc(device_extension_props_len * sizeof(*device_extension_props));
      vkEnumerateDeviceExtensionProperties(physical_devices[j], NULL, &device_extension_props_len, device_extension_props);

      bool found_swapchain_device_extension = false;

      for (u32 k = 0; k < device_extension_props_len; ++k) {
        if (strcmp(device_extension_props[k].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
          found_swapchain_device_extension = true;
          break;
        }
      }

      if (device_extension_props)
        free(device_extension_props);

      // TODO: also check for swapchain capabilites here
      if (graphics_queue_family_index != (u32) -1 &&
          present_queue_family_index != (u32) -1 &&
          found_swapchain_device_extension) {
        physical_device = physical_devices[j];
        found_suitable_physical_device = true;
      }
    }

    if (found_suitable_physical_device)
      break;
  }

  free(physical_devices);

  if (!found_suitable_physical_device) {
    sprintf(error_buffer, "No suitable GPUs were found");
    return NULL;
  }

  float priority = 1.0f;
  u32 queue_create_infos_len;
  VkDeviceQueueCreateInfo *queue_create_infos;

  if (graphics_queue_family_index == present_queue_family_index) {
    queue_create_infos_len = 1;
    queue_create_infos = malloc(queue_create_infos_len * sizeof(*queue_create_infos));
    memset(queue_create_infos, 0, queue_create_infos_len * sizeof(*queue_create_infos));
    queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[0].queueFamilyIndex = graphics_queue_family_index;
    queue_create_infos[0].queueCount = 1;
    queue_create_infos[0].pQueuePriorities = &priority;
  } else {
    queue_create_infos_len = 2;
    queue_create_infos = malloc(queue_create_infos_len * sizeof(*queue_create_infos));
    memset(queue_create_infos, 0, queue_create_infos_len * sizeof(*queue_create_infos));
    queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[0].queueFamilyIndex = graphics_queue_family_index;
    queue_create_infos[0].queueCount = 1;
    queue_create_infos[0].pQueuePriorities = &priority;
    queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[1].queueFamilyIndex = present_queue_family_index;
    queue_create_infos[1].queueCount = 1;
    queue_create_infos[1].pQueuePriorities = &priority;
  }

  const char *device_extensions[] = { "VK_KHR_swapchain" };

  VkPhysicalDeviceFeatures device_features = {0};

  VkDeviceCreateInfo device_create_info = {0};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pQueueCreateInfos = queue_create_infos;
  device_create_info.queueCreateInfoCount = queue_create_infos_len;
  device_create_info.enabledExtensionCount = ARRAY_LEN(device_extensions);
  device_create_info.ppEnabledExtensionNames = device_extensions;
  device_create_info.pEnabledFeatures = &device_features;

  VkDevice device;
  VkResult device_result = vkCreateDevice(physical_device, &device_create_info, NULL, &device);
  if (device_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create logical Vulkan device: %s",
            vk_result_to_cstr(device_result));
    return NULL;
  }

  free(queue_create_infos);

  VkQueue graphics_queue;
  vkGetDeviceQueue(device, graphics_queue_family_index, 0, &graphics_queue);

  VkQueue present_queue;
  vkGetDeviceQueue(device, present_queue_family_index, 0, &present_queue);

  WindowSizeDependantResources resources;
  if (!make_window_size_dependant_resources_except_framebuffers(&resources,
                                                                physical_device, device, surface,
                                                                graphics_queue_family_index,
                                                                present_queue_family_index,
                                                                window->width, window->height))
    return NULL;

  VkSemaphore image_available_semaphore;
  VkSemaphore *render_finished_semaphores = malloc(resources.images_len * sizeof(*render_finished_semaphores));
  VkFence in_flight_fence;

  VkSemaphoreCreateInfo semaphore_create_info = {0};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_create_info = {0};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VkResult sync_result;

  sync_result = vkCreateSemaphore(device, &semaphore_create_info, NULL, &image_available_semaphore);
  if (sync_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan syncronization primitives: %s",
            vk_result_to_cstr(sync_result));
    return NULL;
  }

  sync_result = vkCreateFence(device, &fence_create_info, NULL, &in_flight_fence);
  if (sync_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan syncronization primitives: %s",
            vk_result_to_cstr(sync_result));
    return NULL;
  }

  for (u32 i = 0; i < resources.images_len; ++i) {
    sync_result = vkCreateSemaphore(device, &semaphore_create_info, NULL, render_finished_semaphores + i);
    if (sync_result != VK_SUCCESS) {
      sprintf(error_buffer, "Failed to create Vulkan syncronization primitives: %s",
              vk_result_to_cstr(sync_result));
      return NULL;
    }
  }

  VkCommandPoolCreateInfo command_pool_create_info = {0};
  command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  command_pool_create_info.queueFamilyIndex = graphics_queue_family_index;

  VkCommandPool command_pool;
  VkResult command_pool_result = vkCreateCommandPool(device, &command_pool_create_info, NULL, &command_pool);
  if (command_pool_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create temporary Vulkan command pool: %s",
            vk_result_to_cstr(command_pool_result));
    return NULL;
  }

  VkDescriptorPoolSize descriptor_pool_size = {0};
  descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_pool_size.descriptorCount = 1;

  VkDescriptorPoolCreateInfo descriptor_pool_create_info = {0};
  descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_create_info.poolSizeCount = 1;
  descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
  descriptor_pool_create_info.maxSets = 1;

  VkDescriptorPool descriptor_pool;
  VkResult descriptor_pool_result =
    vkCreateDescriptorPool(device, &descriptor_pool_create_info,
                           NULL, &descriptor_pool);
  if (descriptor_pool_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan descriptor pool: %s",
            vk_result_to_cstr(descriptor_pool_result));
    return NULL;
  }

  VikInstance *result = malloc(sizeof(*result));
  result->window = window;
  result->instance = instance;
  result->surface = surface;
  result->physical_device = physical_device;
  result->graphics_queue_family_index = graphics_queue_family_index;
  result->present_queue_family_index = present_queue_family_index;
  result->device = device;
  result->graphics_queue = graphics_queue;
  result->present_queue = present_queue;
  result->resources = resources;
  result->image_available_semaphore = image_available_semaphore;
  result->render_finished_semaphores = render_finished_semaphores;
  result->in_flight_fence = in_flight_fence;
  result->temp_pool = command_pool;
  result->descriptor_pool = descriptor_pool;
  return result;
}

VikShader *vik_make_shader_vf(VikInstance *instance, Str vertex_bc, Str fragment_bc) {
  VkShaderModuleCreateInfo module_create_info = {0};
  module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  module_create_info.codeSize = vertex_bc.len;
  module_create_info.pCode = (u32 *) vertex_bc.ptr;

  VkResult module_result;

  VkShaderModule vertex_module;
  module_result = vkCreateShaderModule(instance->device, &module_create_info, NULL, &vertex_module);
  if (module_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create vertex shader: %s",
            vk_result_to_cstr(module_result));
    return NULL;
  }

  module_create_info.codeSize = fragment_bc.len;
  module_create_info.pCode = (u32 *) fragment_bc.ptr;

  VkShaderModule fragment_module;
  module_result = vkCreateShaderModule(instance->device, &module_create_info, NULL, &fragment_module);
  if (module_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create fragment shader: %s",
            vk_result_to_cstr(module_result));
    return NULL;
  }

  VikShader *result = malloc(sizeof(*result));
  result->instance = instance;
  result->vertex_module = vertex_module;
  result->fragment_module = fragment_module;
  return result;
}

static u32 get_attr_size(VikAttr attr) {
  switch (attr) {
  case VikAttrFloat:  return sizeof(float) * 1;
  case VikAttrVec2:   return sizeof(float) * 2;
  case VikAttrVec3:   return sizeof(float) * 3;
  case VikAttrVec4:   return sizeof(float) * 4;
  case VikAttrInt:    return sizeof(int) * 1;
  case VikAttrIVec2:  return sizeof(int) * 2;
  case VikAttrIVec3:  return sizeof(int) * 3;
  case VikAttrIVec4:  return sizeof(int) * 4;
  case VikAttrUInt:   return sizeof(unsigned int) * 1;
  case VikAttrUVec2:  return sizeof(unsigned int) * 2;
  case VikAttrUVec3:  return sizeof(unsigned int) * 3;
  case VikAttrUVec4:  return sizeof(unsigned int) * 4;
  }

  return 0;
}

static VkFormat get_attr_format(VikAttr attr) {
  switch (attr) {
  case VikAttrFloat:  return VK_FORMAT_R32_SFLOAT;
  case VikAttrVec2:   return VK_FORMAT_R32G32_SFLOAT;
  case VikAttrVec3:   return VK_FORMAT_R32G32B32_SFLOAT;
  case VikAttrVec4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
  case VikAttrInt:    return VK_FORMAT_R32_SINT;
  case VikAttrIVec2:  return VK_FORMAT_R32G32_SINT;
  case VikAttrIVec3:  return VK_FORMAT_R32G32B32_SINT;
  case VikAttrIVec4:  return VK_FORMAT_R32G32B32A32_SINT;
  case VikAttrUInt:   return VK_FORMAT_R32_UINT;
  case VikAttrUVec2:  return VK_FORMAT_R32G32_UINT;
  case VikAttrUVec3:  return VK_FORMAT_R32G32B32_UINT;
  case VikAttrUVec4:  return VK_FORMAT_R32G32B32A32_UINT;
  }

  return 0;
}

static VkVertexInputBindingDescription get_vertex_binding_desc_for_attrs(VikAttr *attrs, u32 attrs_len) {
  VkVertexInputBindingDescription result = {0};
  result.binding = 0;
  result.stride = 0;
  result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  for (u32 i = 0; i < attrs_len; ++i)
    result.stride += get_attr_size(attrs[i]);

  return result;
}

VkVertexInputAttributeDescription *get_vertex_attr_descs_for_attrs(VikAttr *attrs, u32 attrs_len) {
  VkVertexInputAttributeDescription *result = malloc(attrs_len * sizeof(*result));
  u32 offset = 0;

  for (u32 i = 0; i < attrs_len; ++i) {
    result[i].binding = 0;
    result[i].location = i;
    result[i].format = get_attr_format(attrs[i]);
    result[i].offset = offset;
    offset += get_attr_size(attrs[i]);
  }

  return result;
}

static bool make_buffer(VkPhysicalDevice physical_device, VkDevice device,
                        u32 size, VkBufferUsageFlags usage, u32 memory_prop_flags,
                        VkBuffer *out_buffer, VkDeviceMemory *out_buffer_memory) {
  VkBufferCreateInfo buffer_create_info = {0};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.size = size;
  buffer_create_info.usage = usage;
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult buffer_result = vkCreateBuffer(device, &buffer_create_info, NULL, out_buffer);
  if (buffer_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan buffer: %s",
            vk_result_to_cstr(buffer_result));
    return false;
  }

  VkMemoryRequirements reqs;
  vkGetBufferMemoryRequirements(device, *out_buffer, &reqs);

  VkPhysicalDeviceMemoryProperties props;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &props);

  u32 type_index = (u32) -1;

  for (u32 i = 0; i < props.memoryTypeCount; ++i) {
    if (reqs.memoryTypeBits & (1 << i) &&
        (props.memoryTypes[i].propertyFlags & memory_prop_flags) == memory_prop_flags) {
      type_index = i;
      break;
    }
  }

  if (type_index == (u32) -1) {
    sprintf(error_buffer, "Failed to find suitable memory type");
    return false;
  }

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = reqs.size;
  alloc_info.memoryTypeIndex = type_index;

  VkResult buffer_memory_result = vkAllocateMemory(device, &alloc_info, NULL, out_buffer_memory);
  if (buffer_memory_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to allocate GPU memory: %s",
           vk_result_to_cstr(buffer_memory_result));
    return false;
  }

  vkBindBufferMemory(device, *out_buffer, *out_buffer_memory, 0);

  return true;
}

VikUBO *vik_make_ubo(VikInstance *instance, u32 size) {
  VkBuffer buffer;
  VkDeviceMemory buffer_memory;
  if (!make_buffer(instance->physical_device, instance->device,
                   size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   &buffer, &buffer_memory))
    return NULL;

  void *data;
  vkMapMemory(instance->device, buffer_memory, 0, size, 0, &data);

  VikUBO *result = malloc(sizeof(*result));
  result->instance = instance;
  result->buffer = buffer;
  result->buffer_memory = buffer_memory;
  result->data = data;
  result->size = size;
  return result;
}

VikPipeline *vik_make_pipeline(VikInstance *instance, VikShader *shader,
                               VikAttr *attrs, u32 attrs_len, VikUBO *ubo) {
  VkPipelineShaderStageCreateInfo shader_stage_infos[2] = {0};
  shader_stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shader_stage_infos[0].module = shader->vertex_module;
  shader_stage_infos[0].pName = "main";
  shader_stage_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shader_stage_infos[1].module = shader->fragment_module;
  shader_stage_infos[1].pName = "main";

  VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state = {0};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = ARRAY_LEN(dynamic_states);
  dynamic_state.pDynamicStates = dynamic_states;

  VkVertexInputBindingDescription vertex_binding_desc =
    get_vertex_binding_desc_for_attrs(attrs, attrs_len);

  VkVertexInputAttributeDescription *vertex_attr_descs =
    get_vertex_attr_descs_for_attrs(attrs, attrs_len);

  VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {0};
  vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_create_info.vertexBindingDescriptionCount = 1;
  vertex_input_create_info.pVertexBindingDescriptions = &vertex_binding_desc; // Optional
  vertex_input_create_info.vertexAttributeDescriptionCount = attrs_len;
  vertex_input_create_info.pVertexAttributeDescriptions = vertex_attr_descs; // Optional

  VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {0};
  input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewport_state_create_info = {0};
  viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_create_info.viewportCount = 1;
  viewport_state_create_info.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {0};
  rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer_create_info.depthClampEnable = VK_FALSE;
  rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
  // TODO: make it customizable to be able to draw wireframe/points
  // NOTE: requires extensions
  rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
  // TODO: this one too
  rasterizer_create_info.lineWidth = 1.0f;
  rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer_create_info.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling_create_info = {0};
  multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling_create_info.sampleShadingEnable = VK_FALSE;
  multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling_create_info.minSampleShading = 1.0f; // Optional
  multisampling_create_info.pSampleMask = NULL; // Optional
  multisampling_create_info.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling_create_info.alphaToOneEnable = VK_FALSE; // Optional

  VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
  color_blend_attachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
  color_blend_attachment.blendEnable = VK_TRUE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo color_blending_create_info = {0};
  color_blending_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending_create_info.logicOpEnable = VK_FALSE;
  color_blending_create_info.logicOp = VK_LOGIC_OP_COPY; // Optional
  color_blending_create_info.attachmentCount = 1;
  color_blending_create_info.pAttachments = &color_blend_attachment;

  VkDescriptorSetLayoutBinding layout_binding = {0};
  layout_binding.binding = 0;
  layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layout_binding.descriptorCount = 1;
  layout_binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorSet descriptor_set;
  if (ubo) {
    VkDescriptorSetLayoutCreateInfo layout_create_info = {0};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = 1;
    layout_create_info.pBindings = &layout_binding;

    VkResult descriptor_set_layout_result = vkCreateDescriptorSetLayout(instance->device, &layout_create_info, NULL, &descriptor_set_layout);
    if (descriptor_set_layout_result != VK_SUCCESS) {
      sprintf(error_buffer, "Failed to create Vulkan descriptor set layout: %s",
              vk_result_to_cstr(descriptor_set_layout_result));
      return NULL;
    }

    VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {0};
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool = instance->descriptor_pool;
    descriptor_set_alloc_info.descriptorSetCount = 1;
    descriptor_set_alloc_info.pSetLayouts = &descriptor_set_layout;

    VkResult descriptor_set_result =
      vkAllocateDescriptorSets(instance->device, &descriptor_set_alloc_info, &descriptor_set);
    if (descriptor_set_result != VK_SUCCESS) {
      sprintf(error_buffer, "Failed to create Vulkan descriptor set: %s",
              vk_result_to_cstr(descriptor_set_result));
      return NULL;
    }

    VkDescriptorBufferInfo descriptor_buffer_info = {0};
    descriptor_buffer_info.buffer = ubo->buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = ubo->size;

    VkWriteDescriptorSet descriptor_set_write = {0};
    descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_set_write.dstSet = descriptor_set;
    descriptor_set_write.dstBinding = 0;
    descriptor_set_write.dstArrayElement = 0;
    descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_write.descriptorCount = 1;
    descriptor_set_write.pBufferInfo = &descriptor_buffer_info;

    vkUpdateDescriptorSets(instance->device, 1, &descriptor_set_write, 0, NULL);
  }

  VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
  pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  if (ubo) {
    pipeline_layout_create_info.setLayoutCount = 1; // Optional
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout; // Optional
  } else {
    pipeline_layout_create_info.setLayoutCount = 0; // Optional
    pipeline_layout_create_info.pSetLayouts = NULL; // Optional
  }
  pipeline_layout_create_info.pushConstantRangeCount = 0; // Optional
  pipeline_layout_create_info.pPushConstantRanges = NULL; // Optional

  VkPipelineLayout pipeline_layout;
  VkResult pipeline_layout_result = vkCreatePipelineLayout(instance->device, &pipeline_layout_create_info, NULL, &pipeline_layout);
  if (pipeline_layout_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan pipeline layout: %s",
            vk_result_to_cstr(pipeline_layout_result));
    free(vertex_attr_descs);
    return NULL;
  }

  VkAttachmentDescription color_attachment_desc = {0};
  color_attachment_desc.format = instance->resources.format.format;
  color_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref = {0};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass_desc = {0};
  subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_desc.colorAttachmentCount = 1;
  subpass_desc.pColorAttachments = &color_attachment_ref;

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_create_info = {0};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.attachmentCount = 1;
  render_pass_create_info.pAttachments = &color_attachment_desc;
  render_pass_create_info.subpassCount = 1;
  render_pass_create_info.pSubpasses = &subpass_desc;
  render_pass_create_info.dependencyCount = 1;
  render_pass_create_info.pDependencies = &dependency;

  VkRenderPass render_pass;
  VkResult render_pass_result = vkCreateRenderPass(instance->device, &render_pass_create_info, NULL, &render_pass);
  if (render_pass_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan render pass: %s",
            vk_result_to_cstr(render_pass_result));
    free(vertex_attr_descs);
    return NULL;
  }

  VkGraphicsPipelineCreateInfo pipeline_create_info = {0};
  pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_create_info.stageCount = ARRAY_LEN(shader_stage_infos);
  pipeline_create_info.pStages = shader_stage_infos;
  pipeline_create_info.pVertexInputState = &vertex_input_create_info;
  pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
  pipeline_create_info.pViewportState = &viewport_state_create_info;
  pipeline_create_info.pRasterizationState = &rasterizer_create_info;
  pipeline_create_info.pMultisampleState = &multisampling_create_info;
  pipeline_create_info.pDepthStencilState = NULL; // Optional
  pipeline_create_info.pColorBlendState = &color_blending_create_info;
  pipeline_create_info.pDynamicState = &dynamic_state;
  pipeline_create_info.layout = pipeline_layout;
  pipeline_create_info.renderPass = render_pass;
  pipeline_create_info.subpass = 0;

  VkPipeline graphics_pipeline;
  VkResult pipeline_result = vkCreateGraphicsPipelines(instance->device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &graphics_pipeline);
  if (pipeline_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan graphics pipeline: %s",
            vk_result_to_cstr(pipeline_result));
    free(vertex_attr_descs);
    return NULL;
  }

  VkFramebuffer *framebuffers = malloc(instance->resources.images_len * sizeof(*framebuffers));
  if (!make_framebuffers(framebuffers, &instance->resources, instance->device, render_pass)) {
    free(framebuffers);
    free(vertex_attr_descs);
    return NULL;
  }

  free(vertex_attr_descs);

  VikPipeline *result = malloc(sizeof(*result));
  result->instance = instance;
  result->framebuffers = framebuffers;
  result->descriptor_set_layout = descriptor_set_layout;
  result->descriptor_set = descriptor_set;
  result->layout = pipeline_layout;
  result->render_pass = render_pass;
  result->pipeline = graphics_pipeline;
  result->ubo = ubo;
  return result;
}

VikExecutor *vik_make_executor(VikInstance *instance) {
  VkCommandPoolCreateInfo command_pool_create_info = {0};
  command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  command_pool_create_info.queueFamilyIndex = instance->graphics_queue_family_index;

  VkCommandPool command_pool;
  VkResult command_pool_result = vkCreateCommandPool(instance->device, &command_pool_create_info, NULL, &command_pool);
  if (command_pool_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan command pool: %s",
            vk_result_to_cstr(command_pool_result));
    return NULL;
  }

  VkCommandBufferAllocateInfo command_buffer_alloc_info = {0};
  command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_alloc_info.commandPool = command_pool;
  command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  VkResult command_buffer_result = vkAllocateCommandBuffers(instance->device, &command_buffer_alloc_info, &command_buffer);
  if (command_buffer_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan command buffer: %s",
            vk_result_to_cstr(command_buffer_result));
    return NULL;
  }

  VikExecutor *result = malloc(sizeof(*result));
  result->instance = instance;
  result->pool = command_pool;
  result->buffer = command_buffer;
  return result;
}

static void copy_buffer_content(VkBuffer dest, VkBuffer src,
                                VkDeviceSize size, VkDevice device,
                                VkQueue graphics_queue,
                                VkCommandPool temp_command_pool) {
  VkCommandBufferAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = temp_command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkBufferCopy copy_region = {0};
  copy_region.size = size;
  vkCmdCopyBuffer(command_buffer, src, dest, 1, &copy_region);

  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphics_queue);

  vkFreeCommandBuffers(device, temp_command_pool, 1, &command_buffer);
}

VikMesh *vik_make_mesh_sized(VikInstance *instance, void *data,
                             u32 len, u32 vertex_size,
                             u32 *indices, u32 indices_len) {
  u32 vertex_buffer_size = len * vertex_size;
  u32 index_buffer_size = indices_len * sizeof(u32);

  VkBuffer temp_buffer;
  VkDeviceMemory temp_buffer_memory;
  void *gpu_data;

  if (!make_buffer(instance->physical_device, instance->device,
                   vertex_buffer_size,
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   &temp_buffer, &temp_buffer_memory))
    return NULL;

  vkMapMemory(instance->device, temp_buffer_memory, 0, VK_WHOLE_SIZE, 0, &gpu_data);
  memcpy(gpu_data, data, vertex_buffer_size);
  vkUnmapMemory(instance->device, temp_buffer_memory);

  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;
  if (!make_buffer(instance->physical_device, instance->device,
                   vertex_buffer_size,
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   &vertex_buffer, &vertex_buffer_memory))
    return NULL;

  copy_buffer_content(vertex_buffer, temp_buffer, vertex_buffer_size,
                      instance->device, instance->graphics_queue,
                      instance->temp_pool);

  vkDestroyBuffer(instance->device, temp_buffer, NULL);
  vkFreeMemory(instance->device, temp_buffer_memory, NULL);

  if (!make_buffer(instance->physical_device, instance->device,
                   index_buffer_size,
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   &temp_buffer, &temp_buffer_memory))
    return NULL;

  vkMapMemory(instance->device, temp_buffer_memory, 0, VK_WHOLE_SIZE, 0, &gpu_data);
  memcpy(gpu_data, indices, index_buffer_size);
  vkUnmapMemory(instance->device, temp_buffer_memory);

  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;
  if (!make_buffer(instance->physical_device, instance->device,
                   index_buffer_size,
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   &index_buffer, &index_buffer_memory))
    return NULL;

  copy_buffer_content(index_buffer, temp_buffer, index_buffer_size,
                      instance->device, instance->graphics_queue,
                      instance->temp_pool);

  vkDestroyBuffer(instance->device, temp_buffer, NULL);
  vkFreeMemory(instance->device, temp_buffer_memory, NULL);

  VikMesh *result = malloc(sizeof(*result));
  result->instance = instance;
  result->vertex_buffer = vertex_buffer;
  result->vertex_buffer_memory = vertex_buffer_memory;
  result->index_buffer = index_buffer;
  result->index_buffer_memory = index_buffer_memory;
  result->indices_len = indices_len;
  return result;
}

bool vik_begin_frame(VikExecutor *executor, VikPipeline *pipeline,
                     f32 r, f32 g, f32 b, f32 a) {
  VikInstance *instance = executor->instance;

  vkWaitForFences(instance->device, 1, &instance->in_flight_fence, VK_TRUE, UINT64_MAX);

  VkResult acquire_result = vkAcquireNextImageKHR(instance->device,
                                                  instance->resources.swap_chain,
                                                  UINT64_MAX,
                                                  instance->image_available_semaphore,
                                                  VK_NULL_HANDLE, &instance->image_index);
  if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR) {
    vkDeviceWaitIdle(instance->device);

    for (u32 i = 0; i < instance->resources.images_len; ++i)
      vkDestroyFramebuffer(instance->device, pipeline->framebuffers[i], NULL);

    free(pipeline->framebuffers);

    delete_window_size_dependant_resources(&instance->resources, instance->device);

    bool ok;

    ok = make_window_size_dependant_resources_except_framebuffers(&instance->resources,
                                                                  instance->physical_device,
                                                                  instance->device,
                                                                  instance->surface,
                                                                  instance->graphics_queue_family_index,
                                                                  instance->present_queue_family_index,
                                                                  instance->window->width,
                                                                  instance->window->height);
    if (!ok)
      return false;

    pipeline->framebuffers =
      malloc(instance->resources.images_len * sizeof(*pipeline->framebuffers));
    ok = make_framebuffers(pipeline->framebuffers, &instance->resources,
                           instance->device, pipeline->render_pass);
    if (!ok)
      return false;

    sprintf(error_buffer, "Resized");
    return false;
  }

  vkResetFences(instance->device, 1, &instance->in_flight_fence);

  vkResetCommandBuffer(executor->buffer, 0);

  VkCommandBufferBeginInfo command_buffer_begin_info = {0};
  command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  command_buffer_begin_info.flags = 0; // Optional
  command_buffer_begin_info.pInheritanceInfo = NULL; // Optional

  VkResult begin_result = vkBeginCommandBuffer(executor->buffer, &command_buffer_begin_info);
  if (begin_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to begin recording Vulkan command buffer: %s",
            vk_result_to_cstr(begin_result));
    return false;
  }

  VkClearValue clear_color = { { { r, g, b, a } } };

  VkRenderPassBeginInfo render_pass_begin_info = {0};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = pipeline->render_pass;
  render_pass_begin_info.framebuffer = pipeline->framebuffers[instance->image_index];
  render_pass_begin_info.renderArea.offset = (VkOffset2D) { 0, 0 };
  render_pass_begin_info.renderArea.extent = instance->resources.extent;
  render_pass_begin_info.clearValueCount = 1;
  render_pass_begin_info.pClearValues = &clear_color;

  vkCmdBeginRenderPass(executor->buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

  return true;
}

bool vik_end_frame(VikExecutor *executor, VikPipeline *pipeline) {
  VikInstance *instance = executor->instance;

  vkCmdEndRenderPass(executor->buffer);

  VkResult end_result = vkEndCommandBuffer(executor->buffer);
  if (end_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to end recording Vulkan command buffer: %s",
            vk_result_to_cstr(end_result));
    return false;
  }

  VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &instance->image_available_semaphore;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &executor->buffer;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = instance->render_finished_semaphores + instance->image_index;

  VkResult submit_result = vkQueueSubmit(instance->graphics_queue, 1, &submit_info, instance->in_flight_fence);
  if (submit_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to submit draw command: %s",
            vk_result_to_cstr(submit_result));
    return false;
  }

  VkPresentInfoKHR present_info = {0};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = instance->render_finished_semaphores + instance->image_index;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &instance->resources.swap_chain;
  present_info.pImageIndices = &instance->image_index;

  VkResult draw_result = vkQueuePresentKHR(instance->present_queue, &present_info);
  if (draw_result == VK_ERROR_OUT_OF_DATE_KHR || draw_result == VK_SUBOPTIMAL_KHR) {
    vkDeviceWaitIdle(instance->device);

    for (u32 i = 0; i < instance->resources.images_len; ++i)
      vkDestroyFramebuffer(instance->device, pipeline->framebuffers[i], NULL);

    free(pipeline->framebuffers);

    delete_window_size_dependant_resources(&instance->resources, instance->device);

    bool ok;

    ok = make_window_size_dependant_resources_except_framebuffers(&instance->resources,
                                                                  instance->physical_device,
                                                                  instance->device,
                                                                  instance->surface,
                                                                  instance->graphics_queue_family_index,
                                                                  instance->present_queue_family_index,
                                                                  instance->window->width,
                                                                  instance->window->height);
    if (!ok)
      return false;

    pipeline->framebuffers =
      malloc(instance->resources.images_len * sizeof(*pipeline->framebuffers));
    ok = make_framebuffers(pipeline->framebuffers, &instance->resources,
                           instance->device, pipeline->render_pass);
    if (!ok)
      return false;
  } else if (draw_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to draw: %s", vk_result_to_cstr(draw_result));
    return false;
  }

  return true;
}

void vik_cmd_use_pipeline(VikExecutor *executor, VikPipeline *pipeline) {
  VikInstance *instance = executor->instance;

  vkCmdBindPipeline(executor->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
  vkCmdBindDescriptorSets(executor->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &pipeline->descriptor_set, 0, NULL);

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = instance->resources.extent.width;
  viewport.height = instance->resources.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(executor->buffer, 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.offset = (VkOffset2D) { 0, 0 };
  scissor.extent = instance->resources.extent;

  vkCmdSetScissor(executor->buffer, 0, 1, &scissor);
}

void vik_cmd_draw(VikExecutor *executor, VikMesh *mesh, u32 instances_len) {
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(executor->buffer, 0, 1, &mesh->vertex_buffer, &offset);
  vkCmdBindIndexBuffer(executor->buffer, mesh->index_buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(executor->buffer, mesh->indices_len, instances_len, 0, 0, 0);
}

void vik_update_ubo(VikUBO *ubo, void *data) {
  memcpy(ubo->data, data, ubo->size);
}

void vik_delete_instance(VikInstance *instance) {
  vkDeviceWaitIdle(instance->device);

  vkDestroyDescriptorPool(instance->device, instance->descriptor_pool, NULL);
  vkDestroyCommandPool(instance->device, instance->temp_pool, NULL);

  vkDestroyFence(instance->device, instance->in_flight_fence, NULL);
  for (u32 i = 0; i < instance->resources.images_len; ++i)
    vkDestroySemaphore(instance->device, instance->render_finished_semaphores[i], NULL);
  vkDestroySemaphore(instance->device, instance->image_available_semaphore, NULL);

  free(instance->render_finished_semaphores);

  delete_window_size_dependant_resources(&instance->resources, instance->device);

  vkDestroyDevice(instance->device, NULL);
  vkDestroySurfaceKHR(instance->instance, instance->surface, NULL);
  vkDestroyInstance(instance->instance, NULL);

  free(instance);
}

void vik_delete_shader(VikShader *shader) {
  vkDeviceWaitIdle(shader->instance->device);

  vkDestroyShaderModule(shader->instance->device, shader->vertex_module, NULL);
  vkDestroyShaderModule(shader->instance->device, shader->fragment_module, NULL);

  free(shader);
}

void vik_delete_ubo(VikUBO *ubo) {
  vkDeviceWaitIdle(ubo->instance->device);

  vkDestroyBuffer(ubo->instance->device, ubo->buffer, NULL);
  vkFreeMemory(ubo->instance->device, ubo->buffer_memory, NULL);

  free(ubo);
}

void vik_delete_pipeline(VikPipeline *pipeline) {
  vkDeviceWaitIdle(pipeline->instance->device);

  for (u32 i = 0; i < pipeline->instance->resources.images_len; ++i)
    vkDestroyFramebuffer(pipeline->instance->device, pipeline->framebuffers[i], NULL);
  vkDestroyPipeline(pipeline->instance->device, pipeline->pipeline, NULL);
  vkDestroyRenderPass(pipeline->instance->device, pipeline->render_pass, NULL);
  vkDestroyPipelineLayout(pipeline->instance->device, pipeline->layout, NULL);
  if (pipeline->ubo)
    vkDestroyDescriptorSetLayout(pipeline->instance->device, pipeline->descriptor_set_layout, NULL);

  free(pipeline->framebuffers);
  free(pipeline);
}

void vik_delete_executor(VikExecutor *executor) {
  vkDeviceWaitIdle(executor->instance->device);

  vkDestroyCommandPool(executor->instance->device, executor->pool, NULL);

  free(executor);
}

void vik_delete_mesh(VikMesh *mesh) {
  vkDeviceWaitIdle(mesh->instance->device);

  vkDestroyBuffer(mesh->instance->device, mesh->vertex_buffer, NULL);
  vkFreeMemory(mesh->instance->device, mesh->vertex_buffer_memory, NULL);
  vkDestroyBuffer(mesh->instance->device, mesh->index_buffer, NULL);
  vkFreeMemory(mesh->instance->device, mesh->index_buffer_memory, NULL);

  free(mesh);
}

const char *vik_get_error_str(void) {
  return error_buffer;
}
