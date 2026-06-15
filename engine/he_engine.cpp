#include "he_engine.h"
#include "he_initializers.h"
#include "he_images.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vkbootstrap/VkBootstrap.h>

#include <cassert>
#include <chrono>
#include <thread>

HeapEngine *loadedEngine = nullptr;

// Returns the globally loaded engine instance.
HeapEngine &HeapEngine::Get() { return *loadedEngine; }

void HeapEngine::init()
{
  assert(loadedEngine == nullptr);
  loadedEngine = this;

  if (!glfwInit())
  {
    assert(false && "Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  _window = glfwCreateWindow(_windowExtent.width, _windowExtent.height,
                             "Heap Engine", nullptr, nullptr);
  assert(_window != nullptr && "Failed to create GLFW window");

  // Initialize all Vulkan and rendering subsystems before starting the main loop.
  init_vulkan();
  init_swapchain();
  init_commands();
  init_sync_structures();
  init_descriptors();
  init_pipelines();
  init_imgui();

  _isInitialized = true;
}

void HeapEngine::cleanup()
{
  if (!_isInitialized)
  {
    loadedEngine = nullptr;
    return;
  }

  // Wait for the GPU to finish work before destroying resources.
  vkDeviceWaitIdle(_device);

  for (unsigned int i = 0; i < FRAME_OVERLAP; i++)
  {
    vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
    vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
    vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
    _frames[i]._deletionQueue.flush();
  }

  for (VkSemaphore semaphore : _renderFinishedSemaphores)
  {
    vkDestroySemaphore(_device, semaphore, nullptr);
  }

  _mainDeletionQueue.flush();
  destroy_swapchain();

  vkDestroySurfaceKHR(_instance, _surface, nullptr);
  vkDestroyDevice(_device, nullptr);
  vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
  vkDestroyInstance(_instance, nullptr);

  if (_window)
  {
    glfwDestroyWindow(_window);
    _window = nullptr;
  }
  glfwTerminate();
  loadedEngine = nullptr;
}

void HeapEngine::draw()
{
  // Synchronize with the GPU and reuse per-frame resources before rendering the next frame.
  VK_CHECK(vkWaitForFences(_device, 1, &getCurrentFrame()._renderFence, true, 1000000000));
  getCurrentFrame()._deletionQueue.flush();
  VK_CHECK(vkResetFences(_device, 1, &getCurrentFrame()._renderFence));

  uint32_t swapchainImageIndex;
  VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000,
                                 getCurrentFrame()._swapchainSemaphore,
                                 nullptr, &swapchainImageIndex));

  VkSemaphore imageAvailableSemaphore = getCurrentFrame()._swapchainSemaphore;
  VkSemaphore renderFinishedSemaphore = _renderFinishedSemaphores[swapchainImageIndex];
  VkCommandBuffer cmd = getCurrentFrame()._mainCommandBuffer;

  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  _drawExtent.width = _drawImage.imageExtent.width;
  _drawExtent.height = _drawImage.imageExtent.height;

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
  draw_background(cmd);

  // execute a copy from the draw image into the swapchain
  vkutil::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

  // set swapchain image layout to Attachment Optimal so we can draw it
  vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  // draw imgui into the swapchain image
  draw_imgui(cmd, _swapchainImageViews[swapchainImageIndex]);

  // set swapchain image layout to Present so we can draw it
  vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // finalize the command buffer (we can no longer add commands, but it can now be executed)
  VK_CHECK(vkEndCommandBuffer(cmd));

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
  VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, imageAvailableSemaphore);
  VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, renderFinishedSemaphore);
  VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

  VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, getCurrentFrame()._renderFence));

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = nullptr;
  presentInfo.pSwapchains = &_swapchain;
  presentInfo.swapchainCount = 1;
  presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pImageIndices = &swapchainImageIndex;

  VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
  _frameNumber++;
}

void HeapEngine::run()
{
  bool bQuit = false;

  // Main application loop: process window events and render frames until exit.
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

    if (stop_rendering)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    // Imgui new frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Test ImGui UI
    ImGui::ShowDemoWindow();

    ImGui::Render();

    draw();
  }
}
