
#pragma once

#include <vulkan/vulkan.h>

namespace vkutil
{
  // Transition an image between layouts for GPU operations.
  void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

  // Copy image contents from one Vulkan image to another using a blit.
  void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
};