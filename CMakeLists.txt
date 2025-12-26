cmake_minimum_required(VERSION 3.20)
project(CutieCoCo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(CUTIECOCO_BUILD_QT "Build Qt cross-platform application" ON)
option(CUTIECOCO_BUILD_WINDOWS "Build Windows native application" OFF)
option(CUTIECOCO_BUILD_TESTS "Build unit tests" ON)

# Auto-enable Windows native build on Windows if Qt is disabled
if(WIN32 AND NOT CUTIECOCO_BUILD_QT)
    set(CUTIECOCO_BUILD_WINDOWS ON)
endif()

# Qt-specific configuration
if(CUTIECOCO_BUILD_QT)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Multimedia)
endif()

# Platform-independent emulation library
add_subdirectory(emulation)

# Qt platform application
if(CUTIECOCO_BUILD_QT)
    add_subdirectory(platforms/qt)
endif()

# Windows native application
if(CUTIECOCO_BUILD_WINDOWS AND WIN32)
    add_subdirectory(platforms/windows)
endif()

# Testing
if(CUTIECOCO_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)

    enable_testing()
    add_subdirectory(tests)
endif()
