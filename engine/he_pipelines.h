#pragma once
#include "he_types.h"

namespace vkutil
{
  bool load_shader_module(const char *filePath,
                          VkDevice device,
                          VkShaderModule *outShaderModule);
};