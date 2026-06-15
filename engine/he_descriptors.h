#pragma once

#include "he_types.h"

//> descriptor_allocator
struct DescriptorAllocator
{

  struct PoolSizeRatio
  {
    VkDescriptorType type;
    float ratio;
  };

  VkDescriptorPool pool;

  // Create a descriptor pool sized for the requested descriptor sets.
  void init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
  // Reset allocated descriptor sets so the pool can be reused.
  void clear_descriptors(VkDevice device);
  // Destroy the descriptor pool and release all descriptor resources.
  void destroy_pool(VkDevice device);

  // Allocate a descriptor set from the pool using the provided layout.
  VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};
//< descriptor_allocator

//> descriptor_layout
struct DescriptorLayoutBuilder
{
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  void add_binding(uint32_t binding, VkDescriptorType type);
  void clear();
  VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};
//< descriptor_layout