// he_engine.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "he_types.h"

struct DeletionQueue
{
  std::deque<std::function<void()>> deletors;

  void push_function(std::function<void()> &&function)
  {
    deletors.push_back(function);
  }

  void flush()
  {
    // reverse iterate the deletion queue to execute all the functions
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
    {
      (*it)(); // call functors
    }

    deletors.clear();
  }
};

// Frame data
struct frameData
{
  VkSemaphore _swapchainSemaphore = VK_NULL_HANDLE;
  VkSemaphore _renderSemaphore = VK_NULL_HANDLE;
  VkFence _renderFence = VK_NULL_HANDLE;

  VkCommandPool _commandPool = VK_NULL_HANDLE;
  VkCommandBuffer _mainCommandBuffer = VK_NULL_HANDLE;

  DeletionQueue _deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

struct GLFWwindow;

class HeapEngine
{
public:
  bool _isInitialized{false};
  int _frameNumber{0};
  bool stop_rendering{false};
  VkExtent2D _windowExtent{1700, 900};

  GLFWwindow *_window{nullptr};

  VkInstance _instance;                      // Vulkan library handle
  VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
  VkPhysicalDevice _chosenGPU;               // GPU chosen as the default device
  VkDevice _device;                          // Vulkan device for commands
  VkSurfaceKHR _surface;                     // Vulkan window surface

  // Queues
  frameData _frames[FRAME_OVERLAP];

  frameData &getCurrentFrame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

  VkQueue _graphicsQueue;
  uint32_t _graphicsQueueFamily;

  // Swapchain init
  VkSwapchainKHR _swapchain;
  VkFormat _swapchainImageFormat;

  std::vector<VkImage> _swapchainImages;
  std::vector<VkImageView> _swapchainImageViews;
  VkExtent2D _swapchainExtent;

  std::vector<VkSemaphore> _renderFinishedSemaphores;

  DeletionQueue _mainDeletionQueue;

  VmaAllocator _allocator;

  // Draw resources
  AllocatedImage _drawImage;
  VkExtent2D _drawExtent;

  static HeapEngine &Get();

  // initializes everything in the engine
  void init();

  // shuts down the engine
  void cleanup();

  // draw loop
  void draw();

  // run main loop
  void run();

private:
  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();
  void create_swapchain(uint32_t width, uint32_t height);
  void destroy_swapchain();
  void draw_background(VkCommandBuffer cmd);
};
