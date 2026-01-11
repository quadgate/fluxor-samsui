#!/bin/bash
# Build script for native C++ library
# This script builds the native library using Gradle

set -e  # Exit on error

echo "========================================"
echo "Building Native C++ Library"
echo "========================================"
echo ""

# Check if gradlew exists
if [ ! -f "gradlew" ]; then
    echo "Error: gradlew not found!"
    echo "Please run this script from the project root directory."
    exit 1
fi

# Make gradlew executable
chmod +x gradlew

# Parse command line arguments
BUILD_TYPE="debug"
CLEAN=false

for arg in "$@"; do
    case $arg in
        clean)
            CLEAN=true
            ;;
        release)
            BUILD_TYPE="release"
            ;;
        debug)
            BUILD_TYPE="debug"
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Usage: $0 [clean] [debug|release]"
            exit 1
            ;;
    esac
done

echo "Build Type: $BUILD_TYPE"
echo ""

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "Cleaning previous build..."
    ./gradlew clean
    echo ""
fi

# Build native library
echo "Building native library ($BUILD_TYPE)..."
echo ""

if [ "$BUILD_TYPE" = "release" ]; then
    ./gradlew assembleRelease -x lint
else
    ./gradlew assembleDebug -x lint
fi

if [ $? -ne 0 ]; then
    echo ""
    echo "========================================"
    echo "Build FAILED!"
    echo "========================================"
    exit 1
fi

echo ""
echo "========================================"
echo "Build SUCCESS!"
echo "========================================"
echo ""
echo "Native library location:"
if [ "$BUILD_TYPE" = "release" ]; then
    echo "  app/build/intermediates/cmake/release/obj/"
else
    echo "  app/build/intermediates/cmake/debug/obj/"
fi
echo ""
