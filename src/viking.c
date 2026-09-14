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

typedef Da(VikPipeline *) VikPipelines;

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
  VkQueue                       compute_queue;
  WindowSizeDependantResources  resources;
  VkSemaphore                   image_available_semaphore;
  VkSemaphore                  *render_finished_semaphores;
  VkSemaphore                   compute_finished_semaphore;
  VkFence                       in_flight_fence;
  VkCommandPool                 temp_pool;
  VikPipelines                  graphics_pipelines;
  u32                           image_index;
  bool                          has_compute;
};

typedef enum {
  VikShaderKindVF = 0,
  VikShaderKindVGF,
  VikShaderKindC,
} VikShaderKind;

struct VikShader {
  VikInstance    *instance;
  VkShaderModule  vertex_module;
  VkShaderModule  geometry_module;
  VkShaderModule  fragment_module;
  VkShaderModule  compute_module;
  VikShaderKind   kind;
};

struct VikBuffer {
  VikInstance    *instance;
  VkBuffer        buffer;
  VkDeviceMemory  buffer_memory;
  void           *data;
  u32             size;
  VikBufferKind   kind;
};

struct VikPipeline {
  VikInstance           *instance;
  VkImage                depth_image;
  VkDeviceMemory         depth_image_memory;
  VkImageView            depth_image_view;
  VkFramebuffer         *framebuffers;
  VkDescriptorPool       descriptor_pool;
  VkDescriptorSetLayout  descriptor_set_layout;
  VkDescriptorSet        descriptor_set;
  VkPipelineLayout       layout;
  VkRenderPass           render_pass;
  VkPipeline             pipeline;
  bool                   has_descriptor_set_layout;
  bool                   is_compute;
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

struct VikImage {
  VikInstance    *instance;
  VkImage         image;
  VkDeviceMemory  memory;
  VkImageView     view;
  VkSampler       sampler;
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
                              VkDevice device, VkRenderPass render_pass,
                              VkImageView depth_image_view) {
  for (u32 i = 0; i < resources->images_len; ++i) {
    VkImageView attachments[2] = { resources->image_views[i], depth_image_view };

    VkFramebufferCreateInfo framebuffer_create_info = {0};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = render_pass;
    framebuffer_create_info.attachmentCount = ARRAY_LEN(attachments);
    framebuffer_create_info.pAttachments = attachments;
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

VikInstance *vik_make_instance(WinxWindow *window, VikRequestFlags request) {
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

      VkPhysicalDeviceFeatures device_features;
      vkGetPhysicalDeviceFeatures(physical_devices[j], &device_features);

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
        if ((queue_family_props[k].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            (((request & VikRequestFlagsCompute) == 0) ||
             (queue_family_props[k].queueFlags & VK_QUEUE_COMPUTE_BIT))) {
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
          found_swapchain_device_extension &&
          device_features.samplerAnisotropy &&
          device_features.vertexPipelineStoresAndAtomics &&
          ((request & VikRequestFlagsGeometry) == 0 ||
           device_features.geometryShader)) {
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
  device_features.samplerAnisotropy = VK_TRUE;
  device_features.vertexPipelineStoresAndAtomics = VK_TRUE;
  if (request & VikRequestFlagsGeometry)
    device_features.geometryShader = VK_TRUE;

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

  VkQueue compute_queue;
  if (request & VikRequestFlagsCompute)
    vkGetDeviceQueue(device, graphics_queue_family_index, 0, &compute_queue);

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
  result->compute_queue = compute_queue;
  result->resources = resources;
  result->image_available_semaphore = image_available_semaphore;
  result->render_finished_semaphores = render_finished_semaphores;
  result->in_flight_fence = in_flight_fence;
  result->temp_pool = command_pool;
  result->graphics_pipelines = (VikPipelines) {0};
  result->has_compute = false;
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
  result->kind = VikShaderKindVF;
  return result;
}

VikShader *vik_make_shader_vgf(VikInstance *instance, Str vertex_bc,
                                 Str geometry_bc, Str fragment_bc) {
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

  module_create_info.codeSize = geometry_bc.len;
  module_create_info.pCode = (u32 *) geometry_bc.ptr;

  VkShaderModule geometry_module;
  module_result = vkCreateShaderModule(instance->device, &module_create_info, NULL, &geometry_module);
  if (module_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create geometry shader: %s",
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
  result->geometry_module = vertex_module;
  result->fragment_module = fragment_module;
  result->kind = VikShaderKindVGF;
  return result;
}

VikShader *vik_make_shader_c(VikInstance *instance, Str compute_bc) {
  VkShaderModuleCreateInfo module_create_info = {0};
  module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  module_create_info.codeSize = compute_bc.len;
  module_create_info.pCode = (u32 *) compute_bc.ptr;

  VkResult module_result;

  VkShaderModule compute_module;
  module_result = vkCreateShaderModule(instance->device, &module_create_info, NULL, &compute_module);
  if (module_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create vertex shader: %s",
            vk_result_to_cstr(module_result));
    return NULL;
  }

  VikShader *result = malloc(sizeof(*result));
  result->instance = instance;
  result->compute_module = compute_module;
  result->kind = VikShaderKindC;
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

static bool alloc(VkPhysicalDevice physical_device, VkDevice device,
                  u32 memory_prop_flags, VkMemoryRequirements reqs,
                  VkDeviceMemory *out_buffer_memory) {
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

  return true;
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

  if (!alloc(physical_device, device, memory_prop_flags, reqs, out_buffer_memory))
    return false;

  vkBindBufferMemory(device, *out_buffer, *out_buffer_memory, 0);

  return true;
}

static VkBufferUsageFlagBits get_vulkan_buffer_usage_for_buffer_kind(VikBufferKind kind) {
  switch (kind) {
  case VikBufferKindUBO:  return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  case VikBufferKindSSBO: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }

  return 0;
}

VikBuffer *vik_make_buffer(VikInstance *instance, u32 size, VikBufferKind kind) {
  VkBuffer buffer;
  VkDeviceMemory buffer_memory;
  if (!make_buffer(instance->physical_device, instance->device,
                   size, get_vulkan_buffer_usage_for_buffer_kind(kind),
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   &buffer, &buffer_memory))
    return NULL;

  void *data;
  vkMapMemory(instance->device, buffer_memory, 0, size, 0, &data);

  VikBuffer *result = malloc(sizeof(*result));
  result->instance = instance;
  result->buffer = buffer;
  result->buffer_memory = buffer_memory;
  result->data = data;
  result->size = size;
  result->kind = kind;
  return result;
}

bool make_depth_image_and_view(VkPhysicalDevice physical_device, VkDevice device,
                               VkExtent2D extent, VkImage *out_image,
                               VkDeviceMemory *out_memory,
                               VkImageView *out_image_view) {
  VkImageCreateInfo depth_image_create_info = {0};
  depth_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  depth_image_create_info.imageType = VK_IMAGE_TYPE_2D;
  depth_image_create_info.extent.width = extent.width;
  depth_image_create_info.extent.height = extent.height;
  depth_image_create_info.extent.depth = 1;
  depth_image_create_info.mipLevels = 1;
  depth_image_create_info.arrayLayers = 1;
  depth_image_create_info.format = VK_FORMAT_D32_SFLOAT;
  depth_image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  depth_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  depth_image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult depth_image_result = vkCreateImage(device, &depth_image_create_info, NULL, out_image);
  if (depth_image_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan image: %s",
            vk_result_to_cstr(depth_image_result));
    return false;
  }

  VkMemoryRequirements reqs;
  vkGetImageMemoryRequirements(device, *out_image, &reqs);

  if (!alloc(physical_device, device,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, reqs, out_memory))
    return false;

  vkBindImageMemory(device, *out_image, *out_memory, 0);

  VkImageViewCreateInfo depth_image_view_create_info = {0};
  depth_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  depth_image_view_create_info.image = *out_image;
  depth_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depth_image_view_create_info.format = depth_image_create_info.format;
  depth_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depth_image_view_create_info.subresourceRange.baseMipLevel = 0;
  depth_image_view_create_info.subresourceRange.levelCount = 1;
  depth_image_view_create_info.subresourceRange.baseArrayLayer = 0;
  depth_image_view_create_info.subresourceRange.layerCount = 1;

  VkResult depth_image_view_result = vkCreateImageView(device,
                                                       &depth_image_view_create_info,
                                                       NULL, out_image_view);
  if (depth_image_view_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan image view: %s",
            vk_result_to_cstr(depth_image_view_result));
    return false;
  }

  return true;
}

static VkDescriptorType get_vulkan_descriptor_type_for_buffer_kind(VikBufferKind kind) {
  switch (kind) {
  case VikBufferKindUBO:  return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  case VikBufferKindSSBO: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  }

  return 0;
}

// TODO: update bound buffers/images
VikPipeline *vik_make_pipeline(VikInstance *instance, VikShader *shader,
                               VikAttr *attrs, u32 attrs_len,
                               VikBuffer **buffers, u32 buffers_len,
                               VikImage **images, u32 images_len) {
  u32 shader_stage_infos_len;
  VkPipelineShaderStageCreateInfo shader_stage_infos[3] = {0};

  switch (shader->kind) {
  case VikShaderKindVF: {
    shader_stage_infos_len = 2;
    shader_stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_infos[0].module = shader->vertex_module;
    shader_stage_infos[0].pName = "main";
    shader_stage_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_infos[1].module = shader->fragment_module;
    shader_stage_infos[1].pName = "main";
  } break;

  case VikShaderKindVGF: {
    shader_stage_infos_len = 3;
    shader_stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_infos[0].module = shader->vertex_module;
    shader_stage_infos[0].pName = "main";
    shader_stage_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_infos[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    shader_stage_infos[1].module = shader->geometry_module;
    shader_stage_infos[1].pName = "main";
    shader_stage_infos[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_infos[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_infos[2].module = shader->fragment_module;
    shader_stage_infos[2].pName = "main";
  } break;

  case VikShaderKindC: {
    shader_stage_infos_len = 1;
    shader_stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_infos[0].stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_infos[0].module = shader->compute_module;
    shader_stage_infos[0].pName = "main";
  } break;
  }

  VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state = {0};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = ARRAY_LEN(dynamic_states);
  dynamic_state.pDynamicStates = dynamic_states;

  u32 ubos_len = 0;
  u32 ssbos_len = 0;
  for (u32 i = 0; i < buffers_len; ++i) {
    switch (buffers[i]->kind) {
    case VikBufferKindUBO: {
      ++ubos_len;
    } break;

    case VikBufferKindSSBO: {
      ++ssbos_len;
    } break;
    }
  }

  u32 len = 0;
  VkDescriptorPoolSize descriptor_pool_sizes[2] = {0};
  if (ubos_len > 0) {
    descriptor_pool_sizes[len].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_sizes[len].descriptorCount = ubos_len;
    ++len;
  }
  if (ssbos_len > 0) {
    descriptor_pool_sizes[len].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_pool_sizes[len].descriptorCount = ssbos_len;
    ++len;
  }
  if (images_len > 0) {
    descriptor_pool_sizes[len].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_sizes[len].descriptorCount = images_len;
    ++len;
  }

  VkDescriptorPoolCreateInfo descriptor_pool_create_info = {0};
  descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_create_info.poolSizeCount = len;
  descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;
  descriptor_pool_create_info.maxSets = 1;

  VkDescriptorPool descriptor_pool;
  VkResult descriptor_pool_result =
    vkCreateDescriptorPool(instance->device, &descriptor_pool_create_info,
                           NULL, &descriptor_pool);
  if (descriptor_pool_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan descriptor pool: %s",
            vk_result_to_cstr(descriptor_pool_result));
    return NULL;
  }

  bool has_descriptor_set_layout = buffers_len > 0 || images_len > 0;

  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorSet descriptor_set;
  if (has_descriptor_set_layout) {
    VkDescriptorSetLayoutBinding *layout_bindings =
      malloc((buffers_len + images_len) * sizeof(*layout_bindings));
    memset(layout_bindings, 0, (buffers_len + images_len) * sizeof(*layout_bindings));

    for (u32 i = 0; i < buffers_len; ++i) {
      layout_bindings[i].binding = i;
      layout_bindings[i].descriptorType =
        get_vulkan_descriptor_type_for_buffer_kind(buffers[i]->kind);
      layout_bindings[i].descriptorCount = 1;
      layout_bindings[i].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    }

    for (u32 i = 0; i < images_len; ++i) {
      layout_bindings[i + buffers_len].binding = i + buffers_len;
      layout_bindings[i + buffers_len].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      layout_bindings[i + buffers_len].descriptorCount = 1;
      layout_bindings[i + buffers_len].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    }

    VkDescriptorSetLayoutCreateInfo layout_create_info = {0};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = buffers_len + images_len;
    layout_create_info.pBindings = layout_bindings;

    VkResult descriptor_set_layout_result = vkCreateDescriptorSetLayout(instance->device, &layout_create_info, NULL, &descriptor_set_layout);
    if (descriptor_set_layout_result != VK_SUCCESS) {
      sprintf(error_buffer, "Failed to create Vulkan descriptor set layout: %s",
              vk_result_to_cstr(descriptor_set_layout_result));
      free(layout_bindings);
      return NULL;
    }

    free(layout_bindings);

    VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {0};
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool = descriptor_pool;
    descriptor_set_alloc_info.descriptorSetCount = 1;
    descriptor_set_alloc_info.pSetLayouts = &descriptor_set_layout;

    VkResult descriptor_set_result =
      vkAllocateDescriptorSets(instance->device, &descriptor_set_alloc_info, &descriptor_set);
    if (descriptor_set_result != VK_SUCCESS) {
      sprintf(error_buffer, "Failed to create Vulkan descriptor set: %s",
              vk_result_to_cstr(descriptor_set_result));
      return NULL;
    }

    Da(VkDescriptorBufferInfo) descriptor_ubo_infos = {0};
    Da(VkDescriptorBufferInfo) descriptor_ssbo_infos = {0};

    for (u32 i = 0; i < buffers_len; ++i) {
      VkDescriptorBufferInfo descriptor_buffer_info = {0};
      descriptor_buffer_info.buffer = buffers[i]->buffer;
      descriptor_buffer_info.offset = 0;
      descriptor_buffer_info.range = buffers[i]->size;
      switch (buffers[i]->kind) {
      case VikBufferKindUBO: {
        DA_APPEND(descriptor_ubo_infos, descriptor_buffer_info);
      } break;

      case VikBufferKindSSBO: {
        DA_APPEND(descriptor_ssbo_infos, descriptor_buffer_info);
      } break;
      }
    }

    VkDescriptorImageInfo *descriptor_image_infos =
      malloc(images_len * sizeof(*descriptor_image_infos));
    memset(descriptor_image_infos, 0, images_len * sizeof(*descriptor_image_infos));

    for (u32 i = 0; i < images_len; ++i) {
      descriptor_image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      descriptor_image_infos[i].imageView = images[i]->view;
      descriptor_image_infos[i].sampler = images[i]->sampler;
    }

    len = 0;
    VkWriteDescriptorSet descriptor_set_writes[3] = {0};
    if (descriptor_ubo_infos.len > 0) {
      descriptor_set_writes[len].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_set_writes[len].dstSet = descriptor_set;
      descriptor_set_writes[len].dstBinding = len;
      descriptor_set_writes[len].dstArrayElement = 0;
      descriptor_set_writes[len].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptor_set_writes[len].descriptorCount = descriptor_ubo_infos.len;
      descriptor_set_writes[len].pBufferInfo = descriptor_ubo_infos.items;
      ++len;
    }
    if (descriptor_ssbo_infos.len > 0) {
      descriptor_set_writes[len].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_set_writes[len].dstSet = descriptor_set;
      descriptor_set_writes[len].dstBinding = len;
      descriptor_set_writes[len].dstArrayElement = 0;
      descriptor_set_writes[len].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptor_set_writes[len].descriptorCount = descriptor_ssbo_infos.len;
      descriptor_set_writes[len].pBufferInfo = descriptor_ssbo_infos.items;
      ++len;
    }
    if (images_len > 0) {
      descriptor_set_writes[len].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_set_writes[len].dstSet = descriptor_set;
      descriptor_set_writes[len].dstBinding = len;
      descriptor_set_writes[len].dstArrayElement = 0;
      descriptor_set_writes[len].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptor_set_writes[len].descriptorCount = images_len;
      descriptor_set_writes[len].pImageInfo = descriptor_image_infos;
      ++len;
    }

    vkUpdateDescriptorSets(instance->device, len, descriptor_set_writes, 0, NULL);

    if (descriptor_ubo_infos.items)
      free(descriptor_ubo_infos.items);
    if (descriptor_ssbo_infos.items)
      free(descriptor_ssbo_infos.items);
    if (descriptor_image_infos)
      free(descriptor_image_infos);
  }

  VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
  pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  if (has_descriptor_set_layout) {
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
    return NULL;
  }

  bool is_compute = shader->kind == VikShaderKindC;

  VkRenderPass render_pass;
  VkPipeline pipeline;
  VkResult pipeline_result;
  VkImage depth_image;
  VkDeviceMemory depth_image_memory;
  VkImageView depth_image_view;
  VkFramebuffer *framebuffers;
  if (is_compute) {
    VkComputePipelineCreateInfo pipeline_create_info = {0};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.layout = pipeline_layout;
    pipeline_create_info.stage = shader_stage_infos[0];

    pipeline_result = vkCreateComputePipelines(instance->device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline);
  } else {
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
    color_blend_attachment.blendEnable = VK_TRUE;
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

    VkAttachmentDescription attachment_descs[2] = {0};
    attachment_descs[0].format = instance->resources.format.format;
    attachment_descs[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachment_descs[1].format = VK_FORMAT_D32_SFLOAT;
    attachment_descs[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {0};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = {0};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachment_ref;
    subpass_desc.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info = {0};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = ARRAY_LEN(attachment_descs);
    render_pass_create_info.pAttachments = attachment_descs;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_desc;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    VkResult render_pass_result = vkCreateRenderPass(instance->device, &render_pass_create_info, NULL, &render_pass);
    if (render_pass_result != VK_SUCCESS) {
      sprintf(error_buffer, "Failed to create Vulkan render pass: %s",
              vk_result_to_cstr(render_pass_result));
      free(vertex_attr_descs);
      return NULL;
    }

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {0};
    depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;

    VkGraphicsPipelineCreateInfo pipeline_create_info = {0};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = shader_stage_infos_len;
    pipeline_create_info.pStages = shader_stage_infos;
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterizer_create_info;
    pipeline_create_info.pMultisampleState = &multisampling_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
    pipeline_create_info.pColorBlendState = &color_blending_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.layout = pipeline_layout;
    pipeline_create_info.renderPass = render_pass;
    pipeline_create_info.subpass = 0;

    pipeline_result = vkCreateGraphicsPipelines(instance->device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline);

    free(vertex_attr_descs);


    if (!make_depth_image_and_view(instance->physical_device, instance->device,
                                   instance->resources.extent, &depth_image,
                                   &depth_image_memory, &depth_image_view))
      return NULL;

    framebuffers = malloc(instance->resources.images_len * sizeof(*framebuffers));
    if (!make_framebuffers(framebuffers, &instance->resources,
                           instance->device, render_pass,
                           depth_image_view)) {
      free(framebuffers);
      return NULL;
    }
  }

  if (pipeline_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan graphics pipeline: %s",
            vk_result_to_cstr(pipeline_result));
    return NULL;
  }

  if (is_compute) {
    if (!instance->has_compute) {
      VkSemaphoreCreateInfo semaphore_create_info = {0};
      semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      VkResult sync_result = vkCreateSemaphore(instance->device, &semaphore_create_info, NULL, &instance->compute_finished_semaphore);
      if (sync_result != VK_SUCCESS) {
        sprintf(error_buffer, "Failed to create Vulkan syncronization primitives: %s",
                vk_result_to_cstr(sync_result));
        return NULL;
      }
    }

    instance->has_compute = true;
  }

  VikPipeline *result = malloc(sizeof(*result));
  result->instance = instance;
  result->depth_image = depth_image;
  result->depth_image_memory = depth_image_memory;
  result->depth_image_view = depth_image_view;
  result->framebuffers = framebuffers;
  result->descriptor_pool = descriptor_pool;
  result->descriptor_set_layout = descriptor_set_layout;
  result->descriptor_set = descriptor_set;
  result->layout = pipeline_layout;
  result->render_pass = render_pass;
  result->pipeline = pipeline;
  result->has_descriptor_set_layout = has_descriptor_set_layout;
  result->is_compute = is_compute;

  if (!is_compute)
    DA_APPEND(instance->graphics_pipelines, result);

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

static VkCommandBuffer begin_temp_command_buffer(VkDevice device,
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

  return command_buffer;
}

static void end_temp_command_buffer(VkDevice device,
                                    VkQueue graphics_queue,
                                    VkCommandPool temp_command_pool,
                                    VkCommandBuffer command_buffer) {
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphics_queue);

  vkFreeCommandBuffers(device, temp_command_pool, 1, &command_buffer);
}

static void copy_buffer_content(VkBuffer dest, VkBuffer src,
                                VkDeviceSize size, VkDevice device,
                                VkQueue graphics_queue,
                                VkCommandPool temp_command_pool) {
  VkCommandBuffer command_buffer = begin_temp_command_buffer(device, temp_command_pool);

  VkBufferCopy copy_region = {0};
  copy_region.size = size;
  vkCmdCopyBuffer(command_buffer, src, dest, 1, &copy_region);

  end_temp_command_buffer(device, graphics_queue, temp_command_pool, command_buffer);
}

static void copy_buffer_content_to_image(VkImage dest, VkBuffer src,
                                         u32 width, u32 height, VkDevice device,
                                         VkQueue graphics_queue,
                                         VkCommandPool temp_command_pool) {
  VkCommandBuffer command_buffer = begin_temp_command_buffer(device, temp_command_pool);

  VkBufferImageCopy copy_region = {0};
  copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.imageSubresource.layerCount = 1;
  copy_region.imageExtent = (VkExtent3D) { width, height, 1 };
  vkCmdCopyBufferToImage(command_buffer, src, dest,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1, &copy_region);

  end_temp_command_buffer(device, graphics_queue, temp_command_pool, command_buffer);
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

u32 get_image_format_size(VikImageFormat format) {
  switch (format) {
  case VikImageFormatR8:    return 1;
  case VikImageFormatRG8:   return 2;
  case VikImageFormatRGB8:  return 3;
  case VikImageFormatRGBA8: return 4;
  }

  return 0;
}

VkFormat get_image_format_vulkan_format(VikImageFormat format) {
  switch (format) {
  case VikImageFormatR8:    return VK_FORMAT_R8_SRGB;
  case VikImageFormatRG8:   return VK_FORMAT_R8G8_SRGB;
  case VikImageFormatRGB8:  return VK_FORMAT_R8G8B8_SRGB;
  case VikImageFormatRGBA8: return VK_FORMAT_R8G8B8A8_SRGB;
  }

  return 0;
}

VkFilter get_image_filter_vulkan_filter(VikImageFilter filter) {
  switch (filter) {
  case VikImageFilterLinear:  return VK_FILTER_LINEAR;
  case VikImageFilterNearest: return VK_FILTER_NEAREST;
  }

  return 0;
}

static bool change_image_layout(VkImage image, VkImageLayout src, VkImageLayout dest,
                                VkDevice device, VkCommandPool temp_command_pool,
                                VkQueue graphics_queue) {
  VkCommandBuffer command_buffer = begin_temp_command_buffer(device, temp_command_pool);

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = src;
  barrier.newLayout = dest;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags src_stage;
  VkPipelineStageFlags dest_stage;

  if (src == VK_IMAGE_LAYOUT_UNDEFINED && dest == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (src == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dest == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    sprintf(error_buffer, "Unsupported image layout transition");
    return false;
  }

  vkCmdPipelineBarrier(command_buffer, src_stage, dest_stage,
                       0, 0, NULL, 0, NULL, 1, &barrier);

  end_temp_command_buffer(device, graphics_queue, temp_command_pool, command_buffer);

  return true;
}

// TODO: preallocate samplers and reuse them?
VikImage *vik_make_image_ex(VikInstance *instance, void *data,
                            u32 width, u32 height,
                            VikImageFormat format,
                            VikImageFilter filter) {
  u32 size = width * height * get_image_format_size(format);

  VkBuffer temp_buffer;
  VkDeviceMemory temp_buffer_memory;
  void *gpu_data;

  VkImageCreateInfo image_create_info = {0};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.extent.width = width;
  image_create_info.extent.height = height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.format = get_image_format_vulkan_format(format);
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_create_info.usage =
    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkImage image;
  VkResult image_result = vkCreateImage(instance->device, &image_create_info, NULL, &image);
  if (image_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan image: %s",
            vk_result_to_cstr(image_result));
    return NULL;
  }

  if (!make_buffer(instance->physical_device, instance->device,
                   size,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   &temp_buffer, &temp_buffer_memory))
    return NULL;

  vkMapMemory(instance->device, temp_buffer_memory, 0, VK_WHOLE_SIZE, 0, &gpu_data);
  memcpy(gpu_data, data, size);
  vkUnmapMemory(instance->device, temp_buffer_memory);

  VkMemoryRequirements reqs;
  vkGetImageMemoryRequirements(instance->device, image, &reqs);

  VkDeviceMemory memory;
  if (!alloc(instance->physical_device, instance->device,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, reqs, &memory))
    return NULL;

  vkBindImageMemory(instance->device, image, memory, 0);

  change_image_layout(image, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      instance->device, instance->temp_pool,
                      instance->graphics_queue);
  copy_buffer_content_to_image(image, temp_buffer, width, height,
                               instance->device, instance->graphics_queue,
                               instance->temp_pool);
  change_image_layout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      instance->device, instance->temp_pool,
                      instance->graphics_queue);

  vkDestroyBuffer(instance->device, temp_buffer, NULL);
  vkFreeMemory(instance->device, temp_buffer_memory, NULL);

  VkImageViewCreateInfo view_create_info = {0};
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.image = image;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = image_create_info.format;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;

  VkImageView view;
  VkResult view_result = vkCreateImageView(instance->device,
                                           &view_create_info,
                                           NULL, &view);
  if (view_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan image view: %s",
            vk_result_to_cstr(view_result));
    return NULL;
  }

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(instance->physical_device, &props);

  VkSamplerCreateInfo sampler_create_info = {0};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.magFilter = get_image_filter_vulkan_filter(filter);
  sampler_create_info.minFilter = sampler_create_info.minFilter;
  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.anisotropyEnable = VK_TRUE;
  sampler_create_info.maxAnisotropy = props.limits.maxSamplerAnisotropy;
  sampler_create_info.unnormalizedCoordinates = VK_FALSE;
  sampler_create_info.compareEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_create_info.mipLodBias = 0.0f;
  sampler_create_info.minLod = 0.0f;
  sampler_create_info.maxLod = 0.0f;

  VkSampler sampler;
  VkResult sampler_result = vkCreateSampler(instance->device, &sampler_create_info,
                                            NULL, &sampler);
  if (sampler_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to create Vulkan sampler: %s",
            vk_result_to_cstr(sampler_result));
    return NULL;
  }

  VikImage *result = malloc(sizeof(*result));
  result->instance = instance;
  result->image = image;
  result->memory = memory;
  result->view = view;
  result->sampler = sampler;
  return result;
}

bool vik_begin_frame(VikExecutor *executor, f32 r, f32 g, f32 b, f32 a) {
  VikInstance *instance = executor->instance;

  vkWaitForFences(instance->device, 1, &instance->in_flight_fence, VK_TRUE, UINT64_MAX);

  VkResult acquire_result = vkAcquireNextImageKHR(instance->device,
                                                  instance->resources.swap_chain,
                                                  UINT64_MAX,
                                                  instance->image_available_semaphore,
                                                  VK_NULL_HANDLE, &instance->image_index);
  if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR) {
    vkDeviceWaitIdle(instance->device);

    for (u32 i = 0; i < instance->graphics_pipelines.len; ++i) {
      VikPipeline *pipeline = instance->graphics_pipelines.items[i];

      vkDestroyImageView(instance->device, pipeline->depth_image_view, NULL);
      vkDestroyImage(instance->device, pipeline->depth_image, NULL);
      vkFreeMemory(instance->device, pipeline->depth_image_memory, NULL);

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

      ok = make_depth_image_and_view(instance->physical_device, instance->device,
                                     instance->resources.extent, &pipeline->depth_image,
                                     &pipeline->depth_image_memory,
                                     &pipeline->depth_image_view);
      if (!ok)
        return false;

      pipeline->framebuffers =
        malloc(instance->resources.images_len * sizeof(*pipeline->framebuffers));
      ok = make_framebuffers(pipeline->framebuffers, &instance->resources,
                             instance->device, pipeline->render_pass,
                             pipeline->depth_image_view);
      if (!ok)
        return false;
    }

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

  VkClearValue clear_values[2] = {0};
  clear_values[0].color = (VkClearColorValue) { { r, g, b, a } };
  clear_values[1].depthStencil = (VkClearDepthStencilValue) { 1.0, 0.0 };

  for (u32 i = 0; i < instance->graphics_pipelines.len; ++i) {
    VikPipeline *pipeline = instance->graphics_pipelines.items[i];

    VkRenderPassBeginInfo render_pass_begin_info = {0};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = pipeline->render_pass;
    render_pass_begin_info.framebuffer = pipeline->framebuffers[instance->image_index];
    render_pass_begin_info.renderArea.offset = (VkOffset2D) { 0, 0 };
    render_pass_begin_info.renderArea.extent = instance->resources.extent;
    render_pass_begin_info.clearValueCount = ARRAY_LEN(clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(executor->buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
  }

  return true;
}

bool vik_end_frame(VikExecutor *executor) {
  VikInstance *instance = executor->instance;

  vkCmdEndRenderPass(executor->buffer);

  VkResult end_result = vkEndCommandBuffer(executor->buffer);
  if (end_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to end recording Vulkan command buffer: %s",
            vk_result_to_cstr(end_result));
    return false;
  }

  VkSemaphore wait_semaphores[] = {
    instance->image_available_semaphore,
    instance->compute_finished_semaphore,
  };
  VkPipelineStageFlags wait_stages[] = {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
  };
  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1 + instance->has_compute;
  submit_info.pWaitSemaphores = wait_semaphores;
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

    for (u32 i = 0; i < instance->graphics_pipelines.len; ++i) {
      VikPipeline *pipeline = instance->graphics_pipelines.items[i];

      vkDestroyImageView(instance->device, pipeline->depth_image_view, NULL);
      vkDestroyImage(instance->device, pipeline->depth_image, NULL);
      vkFreeMemory(instance->device, pipeline->depth_image_memory, NULL);

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

      ok = make_depth_image_and_view(instance->physical_device, instance->device,
                                     instance->resources.extent, &pipeline->depth_image,
                                     &pipeline->depth_image_memory,
                                     &pipeline->depth_image_view);
      if (!ok)
        return false;

      pipeline->framebuffers =
        malloc(instance->resources.images_len * sizeof(*pipeline->framebuffers));
      ok = make_framebuffers(pipeline->framebuffers, &instance->resources,
                             instance->device, pipeline->render_pass,
                             pipeline->depth_image_view);
      if (!ok)
        return false;
    }
  } else if (draw_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to draw: %s", vk_result_to_cstr(draw_result));
    return false;
  }

  return true;
}

bool vik_begin_compute_frame(VikExecutor *executor) {
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

  return true;
}

bool vik_end_compute_frame(VikExecutor *executor) {
  VikInstance *instance = executor->instance;

  VkResult end_result = vkEndCommandBuffer(executor->buffer);
  if (end_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to end recording Vulkan command buffer: %s",
            vk_result_to_cstr(end_result));
    return false;
  }

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &executor->buffer;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &instance->compute_finished_semaphore;

  VkResult submit_result = vkQueueSubmit(instance->graphics_queue, 1, &submit_info, instance->in_flight_fence);
  if (submit_result != VK_SUCCESS) {
    sprintf(error_buffer, "Failed to submit compute command: %s",
            vk_result_to_cstr(submit_result));
    return false;
  }

  return true;
}

void vik_cmd_use_pipeline(VikExecutor *executor, VikPipeline *pipeline) {
  VikInstance *instance = executor->instance;

  vkCmdBindPipeline(executor->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
  vkCmdBindDescriptorSets(executor->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &pipeline->descriptor_set, 0, NULL);

  if (!pipeline->is_compute) {
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
}

// TODO: vik_cmd_wait_on_buffer with vkCmdPipelineBarrier
// TODO: vik_cmd_wait_on_image with vkCmdPipelineBarrier

void vik_cmd_draw(VikExecutor *executor, VikMesh *mesh, u32 instances_len) {
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(executor->buffer, 0, 1, &mesh->vertex_buffer, &offset);
  vkCmdBindIndexBuffer(executor->buffer, mesh->index_buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(executor->buffer, mesh->indices_len, instances_len, 0, 0, 0);
}

void vik_cmd_compute(VikExecutor *executor, u32 groups_x, u32 groups_y, u32 groups_z) {
  vkCmdDispatch(executor->buffer, groups_x, groups_y, groups_z);
}

void *vik_get_buffer_data(VikBuffer *buffer) {
  return buffer->data;
}

void vik_set_buffer_data(VikBuffer *buffer, void *data) {
  memcpy(buffer->data, data, buffer->size);
}

void vik_delete_instance(VikInstance *instance) {
  vkDeviceWaitIdle(instance->device);

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

  if (instance->graphics_pipelines.items)
    free(instance->graphics_pipelines.items);
  free(instance);
}

void vik_delete_shader(VikShader *shader) {
  vkDeviceWaitIdle(shader->instance->device);

  vkDestroyShaderModule(shader->instance->device, shader->vertex_module, NULL);
  vkDestroyShaderModule(shader->instance->device, shader->fragment_module, NULL);

  free(shader);
}

void vik_delete_buffer(VikBuffer *buffer) {
  vkDeviceWaitIdle(buffer->instance->device);

  vkDestroyBuffer(buffer->instance->device, buffer->buffer, NULL);
  vkFreeMemory(buffer->instance->device, buffer->buffer_memory, NULL);

  free(buffer);
}

void vik_delete_pipeline(VikPipeline *pipeline) {
  vkDeviceWaitIdle(pipeline->instance->device);

  if (!pipeline->is_compute) {
    vkDestroyImageView(pipeline->instance->device, pipeline->depth_image_view, NULL);
    vkDestroyImage(pipeline->instance->device, pipeline->depth_image, NULL);
    vkFreeMemory(pipeline->instance->device, pipeline->depth_image_memory, NULL);
    for (u32 i = 0; i < pipeline->instance->resources.images_len; ++i)
      vkDestroyFramebuffer(pipeline->instance->device, pipeline->framebuffers[i], NULL);
  }
  vkDestroyPipeline(pipeline->instance->device, pipeline->pipeline, NULL);
  if (!pipeline->is_compute)
    vkDestroyRenderPass(pipeline->instance->device, pipeline->render_pass, NULL);
  vkDestroyPipelineLayout(pipeline->instance->device, pipeline->layout, NULL);
  if (pipeline->has_descriptor_set_layout)
    vkDestroyDescriptorSetLayout(pipeline->instance->device, pipeline->descriptor_set_layout, NULL);
  vkDestroyDescriptorPool(pipeline->instance->device, pipeline->descriptor_pool, NULL);

  if (!pipeline->is_compute)
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

void vik_delete_image(VikImage *image) {
  vkDestroySampler(image->instance->device, image->sampler, NULL);
  vkDestroyImageView(image->instance->device, image->view, NULL);
  vkDestroyImage(image->instance->device, image->image, NULL);
  vkFreeMemory(image->instance->device, image->memory, NULL);

  free(image);
}

const char *vik_get_error_str(void) {
  return error_buffer;
}
