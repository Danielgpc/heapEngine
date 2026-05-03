# Cross-platform Vulkan Engine + Game Build System
# Supports: macOS, Linux, Windows (MinGW)

# ==================== PLATFORM DETECTION ====================
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Linux)
    PLATFORM := LINUX
    LIB_EXT := .so
    SHARED_FLAG := -shared
    FPIC := -fPIC
else ifeq ($(UNAME_S),Darwin)
    PLATFORM := MACOS
    LIB_EXT := .dylib
    SHARED_FLAG := -shared
    FPIC := -fPIC
else ifneq (,$(findstring MINGW,$(UNAME_S)))
    PLATFORM := WINDOWS
    LIB_EXT := .dll
    SHARED_FLAG := -shared
    FPIC :=
else ifneq (,$(findstring MSYS,$(UNAME_S)))
    PLATFORM := WINDOWS
    LIB_EXT := .dll
    SHARED_FLAG := -shared
    FPIC :=
else
    PLATFORM := UNKNOWN
    $(error Unsupported platform: $(UNAME_S))
endif

$(info Building for platform: $(PLATFORM) ($(UNAME_S)))

# ==================== COMPILER & FLAGS ====================
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 $(FPIC)

# ==================== VULKAN SDK PATH ====================
VULKAN_SDK ?= $(shell if [ -n "$$VULKAN_SDK" ]; then printf '%s' "$$VULKAN_SDK"; elif [ -d "$(HOME)/VulkanSDK" ]; then ls -d "$(HOME)/VulkanSDK"/* 2>/dev/null | head -n 1; fi)

ifeq ($(VULKAN_SDK),)
    VULKAN_INCLUDE =
    VULKAN_LIB_PATH =
    VULKAN_RUN_PATH =
else
    VULKAN_INCLUDE = -I$(VULKAN_SDK)/Include -I$(VULKAN_SDK)/include
    VULKAN_LIB_PATH = -L$(VULKAN_SDK)/Lib -L$(VULKAN_SDK)/lib -L$(VULKAN_SDK)/macOS/lib
    VULKAN_RUN_PATH = $(VULKAN_SDK)/Lib:$(VULKAN_SDK)/lib:$(VULKAN_SDK)/macOS/lib
endif

# Platform-specific paths and libraries
ifeq ($(PLATFORM),MACOS)
    INCLUDES = -Iengine -Iinclude -Ithird_party -Ithird_party/vma -Ithird_party/fmt/include -Ithird_party/glm -I/opt/homebrew/include $(VULKAN_INCLUDE)
    LIB_PATHS = -L/opt/homebrew/lib -L$(LIB_DIR) $(VULKAN_LIB_PATH)
    LIBS_ENGINE = -lglfw -lvulkan
    LIBS_GAME = -L$(LIB_DIR) -lheap_engine -lglfw -lvulkan
else ifeq ($(PLATFORM),LINUX)
    INCLUDES = -Iengine -Iinclude -Ithird_party -Ithird_party/vma -Ithird_party/fmt/include -Ithird_party/glm -I/usr/include $(VULKAN_INCLUDE)
    LIB_PATHS = -L/usr/lib -L/usr/local/lib -L$(LIB_DIR) $(VULKAN_LIB_PATH)
    LIBS_ENGINE = -lglfw -lvulkan
    LIBS_GAME = -L$(LIB_DIR) -lheap_engine -lglfw -lvulkan
else ifeq ($(PLATFORM),WINDOWS)
    INCLUDES = -Iengine -Iinclude -Ithird_party -Ithird_party/vma -Ithird_party/fmt/include -Ithird_party/glm -IC:/GLFW/include -IC:/VulkanSDK/*/Include $(VULKAN_INCLUDE)
    LIB_PATHS = -LC:/GLFW/lib -LC:/VulkanSDK/*/Lib -L$(LIB_DIR) $(VULKAN_LIB_PATH)
    LIBS_ENGINE = -lglfw3 -lvulkan
    LIBS_GAME = -L$(LIB_DIR) -lheap_engine -lglfw3 -lvulkan
endif

ifeq ($(PLATFORM),MACOS)
    RPATH_FLAGS = -Wl,-rpath,/opt/homebrew/lib
    ifneq ($(VULKAN_SDK),)
        RPATH_FLAGS += -Wl,-rpath,$(VULKAN_SDK)/lib -Wl,-rpath,$(VULKAN_SDK)/Lib -Wl,-rpath,$(VULKAN_SDK)/macOS/lib
    endif
else
    RPATH_FLAGS =
endif

RUN_PATHS = lib
ifeq ($(PLATFORM),MACOS)
    RUN_PATHS := $(RUN_PATHS):/opt/homebrew/lib
endif
ifneq ($(VULKAN_RUN_PATH),)
    RUN_PATHS := $(RUN_PATHS):$(VULKAN_RUN_PATH)
endif

# ==================== DIRECTORY STRUCTURE ====================
ENGINE_DIR = engine
GAME_DIR = game
BIN_DIR = bin
LIB_DIR = lib
INCLUDE_DIR = include

# Engine library
ENGINE_LIB = $(LIB_DIR)/libheap_engine$(LIB_EXT)
ENGINE_SOURCES = $(wildcard $(ENGINE_DIR)/*.cpp)
ENGINE_OBJECTS = $(patsubst $(ENGINE_DIR)/%.cpp,$(BIN_DIR)/engine_%.o,$(ENGINE_SOURCES))

# Game executable
GAME_TARGET = $(BIN_DIR)/heap_engine
GAME_SOURCES = $(wildcard $(GAME_DIR)/*.cpp)
GAME_OBJECTS = $(patsubst $(GAME_DIR)/%.cpp,$(BIN_DIR)/game_%.o,$(GAME_SOURCES))

# ==================== TARGETS ====================
.PHONY: all engine game clean run help

all: engine game

help:
	@echo "Available targets:"
	@echo "  make all     - Build both engine library and game executable"
	@echo "  make engine  - Build engine library only"
	@echo "  make game    - Build game executable (requires engine library)"
	@echo "  make run     - Build and run the game"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make help    - Show this help message"
	@echo ""
	@echo "Current platform: $(PLATFORM)"

engine: $(ENGINE_LIB)

game: $(GAME_TARGET)

$(ENGINE_LIB): $(ENGINE_OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(CXX) $(CXXFLAGS) $(SHARED_FLAG) -o $@ $^ $(LIB_PATHS) $(RPATH_FLAGS) $(LIBS_ENGINE)
	@echo "✓ Engine library built: $@"

$(GAME_TARGET): $(GAME_OBJECTS) $(ENGINE_LIB)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(GAME_OBJECTS) $(LIB_PATHS) $(RPATH_FLAGS) $(LIBS_GAME)
	@echo "✓ Game executable built: $@"

$(BIN_DIR)/engine_%.o: $(ENGINE_DIR)/%.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@echo "  Compiled: $<"

$(BIN_DIR)/game_%.o: $(GAME_DIR)/%.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@echo "  Compiled: $<"

clean:
	rm -rf $(BIN_DIR) $(LIB_DIR)
	@echo "✓ Build artifacts cleaned"

run: all
	@echo "Launching game..."
	LD_LIBRARY_PATH=$(RUN_PATHS):$$LD_LIBRARY_PATH DYLD_LIBRARY_PATH=$(RUN_PATHS):$$DYLD_LIBRARY_PATH ./$(GAME_TARGET)

# ==================== INFO TARGETS ====================
.PHONY: info info-platform info-structure

info: info-platform info-structure

info-platform:
	@echo "=== Platform Information ==="
	@echo "OS: $(UNAME_S)"
	@echo "Machine: $(UNAME_M)"
	@echo "Detected Platform: $(PLATFORM)"
	@echo "Library Extension: $(LIB_EXT)"
	@echo "Shared Flag: $(SHARED_FLAG)"

info-structure:
	@echo ""
	@echo "=== Directory Structure ==="
	@echo "Engine Source: $(ENGINE_DIR)/"
	@echo "Game Source: $(GAME_DIR)/"
	@echo "Build Output: $(BIN_DIR)/"
	@echo "Libraries: $(LIB_DIR)/"
	@echo "Headers: $(INCLUDE_DIR)/"
	@echo ""
	@echo "Engine Library: $(ENGINE_LIB)"
	@echo "Game Executable: $(GAME_TARGET)"
