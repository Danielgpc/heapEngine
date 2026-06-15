#include "he_pipelines.h"
#include <fstream>
#include "he_initializers.h"

bool vkutil::load_shader_module(const char *filePath,
                                VkDevice device,
                                VkShaderModule *outShaderModule)
{
  // Load SPIR-V shader code from disk and create a Vulkan shader module.
  // Accept both the raw path and a fallback under bin/ for deployed shaders.
  std::string shaderPath = filePath;
  std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    std::string fallbackPath = std::string("bin/") + shaderPath;
    file.open(fallbackPath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
      fmt::print("Failed to open shader file '{}' or '{}'\n", shaderPath, fallbackPath);
      return false;
    }
    shaderPath = std::move(fallbackPath);
  }

  // find what the size of the file is by looking up the location of the cursor
  // because the cursor is at the end, it gives the size directly in bytes
  size_t fileSize = (size_t)file.tellg();

  // spirv expects the buffer to be on uint32, so make sure to reserve a int
  // vector big enough for the entire file
  std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

  // put file cursor at beginning
  file.seekg(0);

  // load the entire file into the buffer
  file.read((char *)buffer.data(), fileSize);

  // now that the file is loaded into the buffer, we can close it
  file.close();

  // create a new shader module, using the buffer we loaded
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = nullptr;

  // codeSize has to be in bytes, so multply the ints in the buffer by size of
  // int to know the real size of the buffer
  createInfo.codeSize = buffer.size() * sizeof(uint32_t);
  createInfo.pCode = buffer.data();

  // check that the creation goes well.
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
  {
    return false;
  }
  *outShaderModule = shaderModule;
  return true;
}