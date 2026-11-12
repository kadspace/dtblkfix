cmake_minimum_required(VERSION 3.22)

project(DtBlkFix VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT WIN32)
  message(FATAL_ERROR "DtBlkFix builds for Windows only (MSVC, or MinGW for the tests)")
endif()

option(DTBLKFIX_PLUGIN "Build the VST3 and Standalone with JUCE" ON)
option(DTBLKFIX_TESTS "Build the tests" ON)
set(DTBLKFIX_JUCE_DIR "" CACHE PATH "JUCE 8 checkout (needed for the plugin)")
set(DTBLKFIX_REFERENCE_DIR "" CACHE PATH
    "Checkout of the original engine to compare renders against (tests)")

set(DTBLKFIX_FFTW_DIR "" CACHE PATH "Folder with libfftw3f-3.dll, copied next to the plugin")

include(cmake/DtBlkFixEngine.cmake)
include(cmake/DtBlkFixResources.cmake)

dtblkfix_add_png(dtblkfix_png ${CMAKE_CURRENT_SOURCE_DIR})
dtblkfix_add_engine(dtblkfix_engine ${CMAKE_CURRENT_SOURCE_DIR})

# the engine side of the plugin wrapper (engine headers don't mix with JUCE's)
add_library(dtblkfix_bridge STATIC plugin/EngineBridge.cpp)
target_link_libraries(dtblkfix_bridge PRIVATE dtblkfix_engine)
target_include_directories(dtblkfix_bridge INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/plugin)
dtblkfix_engine_options(dtblkfix_bridge)
target_compile_definitions(dtblkfix_bridge PRIVATE DTBLKFIX_VERSION="${PROJECT_VERSION}")

if(DTBLKFIX_TESTS)
  enable_testing()
endif()

if(DTBLKFIX_PLUGIN)
  add_subdirectory(plugin)
endif()

if(DTBLKFIX_TESTS)
  add_subdirectory(tests)
endif()
