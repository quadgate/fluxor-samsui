# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name.
project("fluxorio")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        thread_manager.cpp
        io_bridge.cpp
        socket_manager.cpp
        message_encryption.cpp)

# Align native libraries to 16 KB boundaries for Android 15+ compatibility
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,-z,max-page-size=16384")

# Specifies libraries CMake should link to your target library.
target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log)