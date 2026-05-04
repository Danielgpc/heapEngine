//> includes
#include "he_engine.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "he_initializers.h"
#include "he_types.h"

#include <vkbootstrap/VkBootstrap.h>

#include <cassert>
#include <chrono>
#include <thread>
//< includes

//> init
constexpr bool bUseValidaionLayers = true;

HeapEngine *loadedEngine = nullptr;

HeapEngine &HeapEngine::Get() { return *loadedEngine; }
void HeapEngine::init()
{
  // only one engine initialization is allowed with the application.
  assert(loadedEngine == nullptr);
  loadedEngine = this;

  // Initialize GLFW
  if (!glfwInit())
  {
    assert(false && "Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  _window = glfwCreateWindow(_windowExtent.width, _windowExtent.height,
                             "Heap Engine", nullptr, nullptr);
  assert(_window != nullptr && "Failed to create GLFW window");

  init_vulkan();

  init_swapchain();

  init_commands();

  init_sync_structures();

  // everything went fine
  _isInitialized = true;
}
//< init

//> extras
void HeapEngine::cleanup()
{
  if (_isInitialized)
  {
    // make sure the gpu has stopped doing its things
    vkDeviceWaitIdle(_device);

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
      vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
    }

    // Destoy swapchain, surface, device, debugger and instance in correct order
    destroy_swapchain();

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);

    // Destroty GLFW window
    if (_window)
    {
      glfwDestroyWindow(_window);
      _window = nullptr;
    }
    glfwTerminate();
  }

  // clear engine pointer
  loadedEngine = nullptr;
}

void HeapEngine::draw()
{
  // nothing yet
}
//< extras

//> drawloop
void HeapEngine::run()
{
  bool bQuit = false;

  // main loop
  while (!bQuit && _window)
  {
    glfwPollEvents();

    if (glfwWindowShouldClose(_window))
    {
      bQuit = true;
      continue;
    }

    if (glfwGetWindowAttrib(_window, GLFW_ICONIFIED))
    {
      stop_rendering = true;
    }
    else
    {
      stop_rendering = false;
    }

    // do not draw if we are minimized
    if (stop_rendering)
    {
      // throttle the speed to avoid the endless spinning
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    draw();
  }
}
//< drawloop

void HeapEngine::init_vulkan()
{
  vkb::InstanceBuilder builder;

  // make the vulkan instance, with basic debug features
  auto inst_ret = builder.set_app_name("Heap engine app")
                      .request_validation_layers(bUseValidaionLayers)
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

  // grab the instance
  _instance = vkb_inst.instance;
  _debug_messenger = vkb_inst.debug_messenger;

  if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create window surface!");
  }

  // Vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  features.dynamicRendering = true;
  features.synchronization2 = true;

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  // use vkbootstrap to select a gpu.
  // We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
  vkb::PhysicalDeviceSelector selector{vkb_inst};
  vkb::PhysicalDevice physicalDevice = selector
                                           .set_minimum_version(1, 3)
                                           .set_required_features_13(features)
                                           .set_required_features_12(features12)
                                           .set_surface(_surface)
                                           .select()
                                           .value();

  // create the final vulkan device
  vkb::DeviceBuilder deviceBuilder{physicalDevice};

  vkb::Device vkbDevice = deviceBuilder.build().value();

  // Get the VkDevice handle used in the rest of a vulkan application
  _device = vkbDevice.device;
  _chosenGPU = physicalDevice.physical_device;

  // use vkbootstrap to get a Graphics queue
  _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}
void HeapEngine::init_swapchain()
{
  create_swapchain(_windowExtent.width, _windowExtent.height);
}

void HeapEngine::create_swapchain(uint32_t width, uint32_t height)
{
  vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

  _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

  vkb::Swapchain vkbSwapchain = swapchainBuilder
                                    //.use_default_format_selection()
                                    .set_desired_format(VkSurfaceFormatKHR{.format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                    // use vsync present mode
                                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                    .set_desired_extent(width, height)
                                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                    .build()
                                    .value();

  _swapchainExtent = vkbSwapchain.extent;
  // store swapchain and its related images
  _swapchain = vkbSwapchain.swapchain;
  _swapchainImages = vkbSwapchain.get_images().value();
  _swapchainImageViews = vkbSwapchain.get_image_views().value();
}
void HeapEngine::destroy_swapchain()
{
  vkDestroySwapchainKHR(_device, _swapchain, nullptr);

  // destroy swapchain resources
  for (int i = 0; i < (int)_swapchainImageViews.size(); i++)
  {
    vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
  }
}

void HeapEngine::init_commands()
{
  // create a command pool for commands submitted to the graphics queue.
  // we also want the pool to allow for resetting of individual command buffers
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for (int i = 0; i < FRAME_OVERLAP; i++)
  {

    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

    // allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
  }
}
void HeapEngine::init_sync_structures()
{
  // nothing yet
}