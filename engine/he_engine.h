// he_engine.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "he_types.h"

struct GLFWwindow;

class HeapEngine
{
public:
	bool _isInitialized{false};
	int _frameNumber{0};
	bool stop_rendering{false};
	VkExtent2D _windowExtent{1700, 900};

	GLFWwindow *_window{nullptr};

	static HeapEngine &Get();

	// initializes everything in the engine
	void init();

	// shuts down the engine
	void cleanup();

	// draw loop
	void draw();

	// run main loop
	void run();
};
