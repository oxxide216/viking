#include <stdio.h>
#include <vulkan/vulkan.h>

#include "shl/shl-defs.h"
#include "shl/shl-log.h"
#define WINX_VULKAN
#include "winx/winx.h"
#include "winx/event.h"
#include "io.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

#define WINDOW_WIDTH  1600
#define WINDOW_HEIGHT 900

#define APP_NAME    "Viking Application"
#define ENGINE_NAME "Viking Engine"

const char *vk_result_to_cstr(VkResult result)
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

int main(void) {
  Winx *winx = winx_init();
  WinxWindow *window = winx_init_window(winx, STR_LIT("Viking"),
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        WinxGraphicsModeVulkan,
                                        NULL);
  window->target_fps = winx_get_refresh_rate(window);

  VkApplicationInfo app_info = {0};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = APP_NAME;
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = ENGINE_NAME;
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  u32 extensions_len;
  const char * const *extensions = winx_get_vulkan_extensions(&extensions_len);

#ifndef NDEBUG
  const char *layers[] = { "VK_LAYER_KHRONOS_validation" };
#endif

  VkInstanceCreateInfo instance_create_info = {0};
  instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo = &app_info;
  instance_create_info.enabledExtensionCount = extensions_len;
  instance_create_info.ppEnabledExtensionNames = extensions;
#ifndef NDEBUG
  instance_create_info.enabledLayerCount = ARRAY_LEN(layers);
  instance_create_info.ppEnabledLayerNames = layers;
#else
  instance_create_info.enabledLayerCount = 0;
  instance_create_info.ppEnabledLayerNames = NULL;
#endif

  VkInstance instance;
  if (vkCreateInstance(&instance_create_info, NULL, &instance) != VK_SUCCESS) {
    ERROR("Failed to create Vulkan instance\n");
    return 1;
  }

  VkSurfaceKHR surface = winx_create_vulkan_surface(window, instance, NULL);

  u32 physical_devices_len;
  vkEnumeratePhysicalDevices(instance, &physical_devices_len, NULL);

  if (physical_devices_len == 0) {
    ERROR("No GPUs supporting Vulkan were found\n");
    return 1;
  }

  VkPhysicalDevice *physical_devices = malloc(physical_devices_len * sizeof(*physical_devices));
  vkEnumeratePhysicalDevices(instance, &physical_devices_len, physical_devices);

  // TODO: proper selection of a device, for example by its discreteness
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
    ERROR("No suitable GPUs were found\n");
    return 1;
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
  if (vkCreateDevice(physical_device, &device_create_info, NULL, &device) != VK_SUCCESS) {
    ERROR("Failed to create logical Vulkan device\n");
    return 1;
  }

  free(queue_create_infos);

  VkQueue graphics_queue;
  vkGetDeviceQueue(device, graphics_queue_family_index, 0, &graphics_queue);

  VkQueue present_queue;
  vkGetDeviceQueue(device, present_queue_family_index, 0, &present_queue);

  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

  u32 formats_len;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_len, NULL);

  VkSurfaceFormatKHR format;

  if (formats_len > 0) {
    VkSurfaceFormatKHR *formats = malloc(formats_len * sizeof(*formats));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_len, formats);

    format = formats[0];
    for (u32 i = 0; i < formats_len; ++i) {
      if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        format = formats[i];
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

  VkExtent2D extent = capabilities.currentExtent;
  if (extent.width == (u32) -1)
    extent = (VkExtent2D) { WINDOW_WIDTH, WINDOW_HEIGHT };

  VkSwapchainCreateInfoKHR swap_chain_create_info = {0};
  swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swap_chain_create_info.surface = surface;
  swap_chain_create_info.minImageCount = capabilities.minImageCount + 1;
  swap_chain_create_info.imageFormat = format.format;
  swap_chain_create_info.imageColorSpace = format.colorSpace;
  swap_chain_create_info.imageExtent = extent;
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

  VkSwapchainKHR swap_chain;
  if (vkCreateSwapchainKHR(device, &swap_chain_create_info, NULL, &swap_chain) != VK_SUCCESS) {
    ERROR("Failed to create Vulkan swap chain\n");
    return 1;
  }

  u32 images_len;
  vkGetSwapchainImagesKHR(device, swap_chain, &images_len, NULL);

  VkImage *images = malloc(images_len * sizeof(*images));
  vkGetSwapchainImagesKHR(device, swap_chain, &images_len, images);

  VkImageView *image_views = malloc(images_len * sizeof(*image_views));
  for (u32 i = 0; i < images_len; ++i) {
    VkImageViewCreateInfo image_view_create_info = {0};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = images[i];
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format.format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &image_view_create_info, NULL, image_views + i) != VK_SUCCESS) {
      ERROR("Failed to create Vulkan image views\n");
      return 1;
    }
  }

  Str vert_bc = read_file("vert.spv");
  Str frag_bc = read_file("frag.spv");

  VkShaderModuleCreateInfo shader_module_create_info = {0};
  shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_module_create_info.codeSize = vert_bc.len;
  shader_module_create_info.pCode = (u32 *) vert_bc.ptr;

  VkShaderModule vert_shader_module;
  if (vkCreateShaderModule(device, &shader_module_create_info, NULL, &vert_shader_module) != VK_SUCCESS) {
    ERROR("Failed to create vertex shader");
    return 1;
  }

  free(vert_bc.ptr);

  shader_module_create_info.codeSize = frag_bc.len;
  shader_module_create_info.pCode = (u32 *) frag_bc.ptr;

  VkShaderModule frag_shader_module;
  if (vkCreateShaderModule(device, &shader_module_create_info, NULL, &frag_shader_module) != VK_SUCCESS) {
    ERROR("Failed to create fragment shader");
    return 1;
  }

  free(frag_bc.ptr);

  VkPipelineShaderStageCreateInfo shader_stage_infos[2] = {0};
  shader_stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shader_stage_infos[0].module = vert_shader_module;
  shader_stage_infos[0].pName = "main";
  shader_stage_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shader_stage_infos[1].module = frag_shader_module;
  shader_stage_infos[1].pName = "main";

  VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state = {0};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = ARRAY_LEN(dynamic_states);
  dynamic_state.pDynamicStates = dynamic_states;

  VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {0};
  vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_create_info.vertexBindingDescriptionCount = 0;
  vertex_input_create_info.pVertexBindingDescriptions = NULL; // Optional
  vertex_input_create_info.vertexAttributeDescriptionCount = 0;
  vertex_input_create_info.pVertexAttributeDescriptions = NULL; // Optional

  VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {0};
  input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = extent.width;
  viewport.height = extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {0};
  scissor.offset = (VkOffset2D) { 0, 0 };
  scissor.extent = extent;

  VkPipelineViewportStateCreateInfo viewport_state_create_info = {0};
  viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_create_info.viewportCount = 1;
  viewport_state_create_info.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {0};
  rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer_create_info.depthClampEnable = VK_FALSE;
  rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
  // TODO: make it customizable to be able to draw wireframe/points
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

  // TODO: blending
  VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
  color_blend_attachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
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

  VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
  pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.setLayoutCount = 0; // Optional
  pipeline_layout_create_info.pSetLayouts = NULL; // Optional
  pipeline_layout_create_info.pushConstantRangeCount = 0; // Optional
  pipeline_layout_create_info.pPushConstantRanges = NULL; // Optional

  VkPipelineLayout pipeline_layout;
  if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout) != VK_SUCCESS) {
    ERROR("Failed to create Vulkan pipeline layout\n");
    return 1;
  }

  VkAttachmentDescription color_attachment_desc = {0};
  color_attachment_desc.format = format.format;
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
  if (vkCreateRenderPass(device, &render_pass_create_info, NULL, &render_pass) != VK_SUCCESS) {
    ERROR("Failed to create Vulkan render pass\n");
    return 1;
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
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &graphics_pipeline) != VK_SUCCESS) {
    ERROR("Failed to create Vulkan graphics pipeline\n");
    return 1;
  }

  vkDestroyShaderModule(device, vert_shader_module, NULL);
  vkDestroyShaderModule(device, frag_shader_module, NULL);

  VkFramebuffer *framebuffers = malloc(images_len * sizeof(*framebuffers));
  for (u32 i = 0; i < images_len; ++i) {
    VkFramebufferCreateInfo framebuffer_create_info = {0};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = render_pass;
    framebuffer_create_info.attachmentCount = 1;
    framebuffer_create_info.pAttachments = image_views + i;
    framebuffer_create_info.width = extent.width;
    framebuffer_create_info.height = extent.height;
    framebuffer_create_info.layers = 1;

    if (vkCreateFramebuffer(device, &framebuffer_create_info, NULL, framebuffers + i) != VK_SUCCESS) {
      ERROR("Failed to create Vulkan framebuffers\n");
      return 1;
    }
  }

  VkCommandPoolCreateInfo command_pool_create_info = {0};
  command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  command_pool_create_info.queueFamilyIndex = graphics_queue_family_index;

  VkCommandPool command_pool;
  if (vkCreateCommandPool(device, &command_pool_create_info, NULL, &command_pool) != VK_SUCCESS) {
    ERROR("Failed to create Vulkan command pool\n");
    return 1;
  }

  VkCommandBufferAllocateInfo command_buffer_alloc_info = {0};
  command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_alloc_info.commandPool = command_pool;
  command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  if (vkAllocateCommandBuffers(device, &command_buffer_alloc_info, &command_buffer) != VK_SUCCESS) {
    ERROR("Failed to create Vulkan command buffer");
    return 1;
  }

  VkSemaphore image_available_semaphore;
  VkSemaphore *render_finished_semaphores = malloc(images_len * sizeof(*render_finished_semaphores));
  VkFence in_flight_fence;

  VkSemaphoreCreateInfo semaphore_create_info = {0};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_create_info = {0};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateSemaphore(device, &semaphore_create_info, NULL, &image_available_semaphore) != VK_SUCCESS ||
      vkCreateFence(device, &fence_create_info, NULL, &in_flight_fence) != VK_SUCCESS) {
    ERROR("Failed to create Vulkan syncronization primitives\n");
    return 1;
  }

  for (u32 i = 0; i < images_len; ++i) {
    if (vkCreateSemaphore(device, &semaphore_create_info, NULL, render_finished_semaphores + i) != VK_SUCCESS) {
      ERROR("Failed to create Vulkan syncronization primitives\n");
      return 1;
    }
  }

  bool is_running = true;

  while (is_running) {
    WinxEvent event;
    while ((event = winx_get_event(window, false)).kind != WinxEventKindNone) {
      is_running = event.kind != WinxEventKindQuit;
      if (!is_running)
        break;
    }

    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fence);

    u32 image_index;
    vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);

    vkResetCommandBuffer(command_buffer, 0);

    VkCommandBufferBeginInfo command_buffer_begin_info = {0};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = 0; // Optional
    command_buffer_begin_info.pInheritanceInfo = NULL; // Optional

    if (vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info) != VK_SUCCESS) {
      ERROR("Failed to begin recording Vulkan command buffer\n");
      return 1;
    }

    VkClearValue clear_color = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };

    VkRenderPassBeginInfo render_pass_begin_info = {0};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.framebuffer = framebuffers[image_index];
    render_pass_begin_info.renderArea.offset = (VkOffset2D) { 0, 0 };
    render_pass_begin_info.renderArea.extent = extent;
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
      ERROR("Failed to end recording Vulkan command buffer\n");
      return 1;
    }

    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphore;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = render_finished_semaphores + image_index;

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence) != VK_SUCCESS) {
      ERROR("Failed to submit draw command\n");
      return 1;
    }

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = render_finished_semaphores + image_index;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swap_chain;
    present_info.pImageIndices = &image_index;

    VkResult draw_result = vkQueuePresentKHR(present_queue, &present_info);
    if (draw_result != VK_SUCCESS)
      ERROR("Failed to draw: %s\n", vk_result_to_cstr(draw_result));

    winx_draw(window);
  }

  vkDeviceWaitIdle(device);

  vkDestroyFence(device, in_flight_fence, NULL);
  for (u32 i = 0; i < images_len; ++i)
    vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
  vkDestroySemaphore(device, image_available_semaphore, NULL);
  vkDestroyCommandPool(device, command_pool, NULL);

  free(render_finished_semaphores);

  for (u32 i = 0; i < images_len; ++i) {
    vkDestroyFramebuffer(device, framebuffers[i], NULL);
    vkDestroyImageView(device, image_views[i], NULL);
  }

  vkDestroyPipeline(device, graphics_pipeline, NULL);
  vkDestroyRenderPass(device, render_pass, NULL);
  vkDestroyPipelineLayout(device, pipeline_layout, NULL);

  free(framebuffers);
  free(image_views);
  free(images);

  vkDestroySwapchainKHR(device, swap_chain, NULL);
  vkDestroyDevice(device, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);

  winx_destroy_window(window);
  winx_cleanup(winx);

  return 0;
}
