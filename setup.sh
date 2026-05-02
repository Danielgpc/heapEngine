#!/bin/bash

# Heap Engine Setup Script
# Sets up environment variables for Vulkan and runs the game

# Set Vulkan SDK path if not already set
if [ -z "$VULKAN_SDK" ]; then
    # Try to find Vulkan SDK in home directory
    VULKAN_SDK_DIR="$HOME/VulkanSDK"
    if [ -d "$VULKAN_SDK_DIR" ]; then
        # Get the latest version (prefer version-numbered directories)
        VULKAN_SDK=$(ls -d "$VULKAN_SDK_DIR"/*/ 2>/dev/null | grep -E '/[0-9]+\.' | sort -V | tail -n 1 | sed 's|/$||')
        if [ -z "$VULKAN_SDK" ]; then
            # Fallback to any directory if no version-numbered ones found
            VULKAN_SDK=$(ls -d "$VULKAN_SDK_DIR"/*/ 2>/dev/null | head -n 1 | sed 's|/$||')
        fi
        if [ -n "$VULKAN_SDK" ] && [ -d "$VULKAN_SDK" ]; then
            export VULKAN_SDK
            echo "Found Vulkan SDK: $VULKAN_SDK"
        fi
    fi
fi

# Set up library paths
PROJECT_LIB_PATH="lib"
HOMEBREW_LIB_PATH="/opt/homebrew/lib"

# Build Vulkan library paths
VULKAN_LIB_PATHS=""
if [ -n "$VULKAN_SDK" ]; then
    VULKAN_LIB_PATHS="$VULKAN_SDK/lib:$VULKAN_SDK/Lib:$VULKAN_SDK/macOS/lib"
fi

# Set LD_LIBRARY_PATH
NEW_LD_LIBRARY_PATH="$PROJECT_LIB_PATH:$HOMEBREW_LIB_PATH"
if [ -n "$VULKAN_LIB_PATHS" ]; then
    NEW_LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH:$VULKAN_LIB_PATHS"
fi
if [ -n "$LD_LIBRARY_PATH" ]; then
    NEW_LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH:$LD_LIBRARY_PATH"
fi
export LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH"

# Set DYLD_LIBRARY_PATH (macOS specific)
NEW_DYLD_LIBRARY_PATH="$PROJECT_LIB_PATH:$HOMEBREW_LIB_PATH"
if [ -n "$VULKAN_LIB_PATHS" ]; then
    NEW_DYLD_LIBRARY_PATH="$NEW_DYLD_LIBRARY_PATH:$VULKAN_LIB_PATHS"
fi
if [ -n "$DYLD_LIBRARY_PATH" ]; then
    NEW_DYLD_LIBRARY_PATH="$NEW_DYLD_LIBRARY_PATH:$DYLD_LIBRARY_PATH"
fi
export DYLD_LIBRARY_PATH="$NEW_DYLD_LIBRARY_PATH"

# Print environment info
echo "Environment setup:"
echo "  VULKAN_SDK: $VULKAN_SDK"
echo "  LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
echo "  DYLD_LIBRARY_PATH: $DYLD_LIBRARY_PATH"
echo ""

# Run the game
echo "Launching Heap Engine..."
./bin/heap_engine