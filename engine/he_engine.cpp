//> includes
#include "he_engine.h"

#include <GLFW/glfw3.h>

#include "he_initializers.h"
#include "he_types.h"

#include <cassert>
#include <chrono>
#include <thread>
//< includes

//> init
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

    // everything went fine
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