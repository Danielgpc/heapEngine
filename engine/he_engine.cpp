//> includes
#include "he_engine.h"

#include <GLFW/glfw3.h>

#include <vk_initializers.h>
#include <vk_types.h>

#include <chrono>
#include <thread>
#include <cassert>
//< includes

//> init
constexpr bool bUseValidationLayers = false;

HeapEngine *loadedEngine = nullptr;

HeapEngine &HeapEngine::Get() { return *loadedEngine; }
void HeapEngine::init()
{
  assert(loadedEngine == nullptr);
  loadedEngine = this;

  if (!glfwInit()) {
    assert(false && "Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  _window = glfwCreateWindow(_windowExtent.width, _windowExtent.height,
                             "Heap Engine", nullptr, nullptr);
  assert(_window != nullptr && "Failed to create GLFW window");

  _isInitialized = true;
}
//< init

//> extras
void HeapEngine::cleanup()
{
  if (_isInitialized)
  {
    if (_window)
    {
      glfwDestroyWindow(_window);
      _window = nullptr;
    }
    glfwTerminate();
  }

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

    draw();
  }
}
//< drawloop
