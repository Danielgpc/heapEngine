#include "he_engine.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vkbootstrap/VkBootstrap.h>

#include "he_initializers.h"
#include "he_images.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

void HeapEngine::init_vulkan()
{
  vkb::InstanceBuilder builder;

  auto inst_ret = builder.set_app_name("Heap engine app")
                      .request_validation_layers(bUseValidationLayers)
                      .use_default_debug_messenger()
                      .require_api_version(1, 3, 0)
                      .set_headless(false)
                      .build();

  if (!inst_ret)
  {
    fmt::println(stderr, "Failed to create Vulkan instance: {}", inst_ret.error().message());
    return;
  }

  vkb::Instance vkb_inst = inst_ret.value();

  _instance = vkb_inst.instance;
  _debug_messenger = vkb_inst.debug_messenger;

  if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create window surface!");
  }

  VkPhysicalDeviceVulkan13Features features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  features.dynamicRendering = true;
  features.synchronization2 = true;

  VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  vkb::PhysicalDeviceSelector selector{vkb_inst};
  vkb::PhysicalDevice physicalDevice = selector
                                           .set_minimum_version(1, 3)
                                           .set_required_features_13(features)
                                           .set_required_features_12(features12)
                                           .set_surface(_surface)
                                           .select()
                                           .value();

  vkb::DeviceBuilder deviceBuilder{physicalDevice};
  vkb::Device vkbDevice = deviceBuilder.build().value();

  _device = vkbDevice.device;
  _chosenGPU = physicalDevice.physical_device;
  _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = _chosenGPU;
  allocatorInfo.device = _device;
  allocatorInfo.instance = _instance;
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  vmaCreateAllocator(&allocatorInfo, &_allocator);

  _mainDeletionQueue.push_function([&]()
                                   { vmaDestroyAllocator(_allocator); });
}

void HeapEngine::init_swapchain()
{
  create_swapchain(_windowExtent.width, _windowExtent.height);

  VkExtent3D drawImageExtent = {
      _windowExtent.width,
      _windowExtent.height,
      1};

  _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
  _drawImage.imageExtent = drawImageExtent;

  VkImageUsageFlags drawImageUsages{};
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

  VmaAllocationCreateInfo rimg_allocinfo = {};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

  VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

  VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

  _mainDeletionQueue.push_function([this]()
                                   {
    vkDestroyImageView(_device, _drawImage.imageView, nullptr);
    vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation); });
}

void HeapEngine::create_swapchain(uint32_t width, uint32_t height)
{
  vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

  _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

  vkb::Swapchain vkbSwapchain = swapchainBuilder
                                    .set_desired_format(VkSurfaceFormatKHR{.format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                    .set_desired_extent(width, height)
                                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                    .build()
                                    .value();

  _swapchainExtent = vkbSwapchain.extent;
  _swapchain = vkbSwapchain.swapchain;
  _swapchainImages = vkbSwapchain.get_images().value();
  _swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void HeapEngine::destroy_swapchain()
{
  vkDestroySwapchainKHR(_device, _swapchain, nullptr);

  for (size_t i = 0; i < _swapchainImageViews.size(); i++)
  {
    vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
  }
}

void HeapEngine::init_commands()
{
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for (unsigned int i = 0; i < FRAME_OVERLAP; i++)
  {
    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
  }
}

void HeapEngine::init_sync_structures()
{
  VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

  const uint32_t imageCount = static_cast<uint32_t>(_swapchainImages.size());
  _renderFinishedSemaphores.resize(imageCount, VK_NULL_HANDLE);

  for (unsigned int i = 0; i < FRAME_OVERLAP; i++)
  {
    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
  }

  for (uint32_t i = 0; i < imageCount; i++)
  {
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[i]));
  }
}
