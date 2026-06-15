#pragma once
#include "he_types.h"

namespace vkutil
{
  // Load a SPIR-V shader from disk and create a Vulkan shader module.
  // Supports both a direct file path and a fallback under bin/.
  bool load_shader_module(const char *filePath,
                          VkDevice device,
                          VkShaderModule *outShaderModule);
};