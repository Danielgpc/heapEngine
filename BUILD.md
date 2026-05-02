# Build Configuration Details

This document explains how the cross-platform build system works and how to configure it for different environments.

## Platform Detection

The Makefile uses `uname -s` to detect the operating system:

```
Linux  → .so library
Darwin → .dylib library (macOS)
MINGW  → .dll library (Windows)
MSYS   → .dll library (Windows)
```

## Compiler Flags

### Position Independent Code (-fPIC)

Shared libraries on Unix-like systems require Position Independent Code:

- **macOS & Linux**: `-fPIC` is enabled
- **Windows**: Not required (Windows PE format handles this differently)

### Shared Library Flag

- **macOS & Linux**: `-shared`
- **Windows**: `-shared` (works with MinGW)

## Platform-Specific Include Paths

### macOS (Homebrew)

```makefile
INCLUDES = -Iinclude -I/opt/homebrew/include
```

### Linux (System paths)

```makefile
INCLUDES = -Iinclude -I/usr/include
```

### Windows (Custom paths)

```makefile
INCLUDES = -Iinclude -IC:/GLFW/include -IC:/VulkanSDK/*/Include
```

## Library Linking

### Runtime Library Path Resolution

The `make run` target sets environment variables for runtime library loading:

```bash
LD_LIBRARY_PATH=lib:$LD_LIBRARY_PATH      # Linux
DYLD_LIBRARY_PATH=lib:$DYLD_LIBRARY_PATH  # macOS
```

This allows the executable to find `lib/libheap_engine.*` at runtime.

## Object File Organization

To avoid conflicts, object files are prefixed with their module:

- `bin/engine_*.o` → Engine object files
- `bin/game_*.o` → Game object files

## Cross-Compilation

To cross-compile for a different platform:

```bash
# Example: Compile for Linux from macOS (requires Linux target toolchain)
make CXX=x86_64-linux-gnu-g++ clean all
```

## Customization

### Change Output Directories

Edit these variables in the Makefile:

```makefile
BIN_DIR = bin        # Change to build/
LIB_DIR = lib        # Change to build/lib/
INCLUDE_DIR = include # Change if headers move
```

### Change Library Name

```makefile
ENGINE_LIB = $(LIB_DIR)/libheap_engine$(LIB_EXT)
# Change "libheap_engine" to desired name
```

### Change Executable Name

```makefile
GAME_TARGET = $(BIN_DIR)/heap_engine
# Change "heap_engine" to desired name
```

## Static vs. Dynamic Linking

### Current Setup (Dynamic)

The engine is compiled as a **shared library** (`.dylib`, `.so`, `.dll`) and the game links to it dynamically. This allows:
- Smaller executable size
- Hot replacement of engine code during development
- Reuse of engine library by multiple games

### To Switch to Static Linking

1. Change the Makefile rule:

```makefile
# Instead of:
$(ENGINE_LIB): $(ENGINE_OBJECTS)
    $(CXX) $(CXXFLAGS) $(SHARED_FLAG) ...

# Use:
$(ENGINE_LIB): $(ENGINE_OBJECTS)
    ar rcs $@ $^
```

2. Link the game with the static library:

```makefile
$(GAME_TARGET): $(GAME_OBJECTS) $(ENGINE_LIB)
    $(CXX) $(CXXFLAGS) -o $@ $(GAME_OBJECTS) $(ENGINE_LIB) $(LIBS_GAME)
```

## Troubleshooting

### "cannot find -lheap_engine"

Ensure the library was built:
```bash
make engine
ls -la lib/libheap_engine.*
```

### "library not found for -lglfw"

Install GLFW:
- macOS: `brew install glfw`
- Linux: `sudo apt-get install libglfw3-dev`
- Windows: Download and install GLFW dev package

### Runtime: "dyld: Library not loaded"

Set library path and run:
```bash
make run
# OR manually:
DYLD_LIBRARY_PATH=lib ./bin/heap_engine
```

### On Linux: "error while loading shared libraries"

```bash
LD_LIBRARY_PATH=lib ./bin/heap_engine
# OR add to system library path:
sudo ldconfig -n lib
```
