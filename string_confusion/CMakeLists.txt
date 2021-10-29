# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("minidl_encrypt")



# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(INCLUDE_DIR ./include)
set(SRC_DIR ./src)
include_directories(${INCLUDE_DIR} ${SRC_DIR})

add_library( # Sets the name of the library.
             caculator

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             ./src/caculator.cpp
        )


add_executable(${PROJECT_NAME} ./src/encrypt.cpp)

add_executable(main ./src/main.cpp)
