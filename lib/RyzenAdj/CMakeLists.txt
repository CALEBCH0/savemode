cmake_minimum_required(VERSION 3.10)
project(ryzenadj_standalone VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Library sources
set(RYZENADJ_SOURCES
    ryzenadj.c
    smu_ops.c
)

set(RYZENADJ_HEADERS
    ryzenadj.h
    ryzenadj_priv.h
)

# Create static library
add_library(ryzenadj_static STATIC ${RYZENADJ_SOURCES})
target_include_directories(ryzenadj_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Create shared library
add_library(ryzenadj_shared SHARED ${RYZENADJ_SOURCES})
target_include_directories(ryzenadj_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set output names
set_target_properties(ryzenadj_static PROPERTIES OUTPUT_NAME ryzenadj)
set_target_properties(ryzenadj_shared PROPERTIES OUTPUT_NAME ryzenadj)

# Platform-specific settings
if(WIN32)
    # Windows specific libraries and settings
    target_link_libraries(ryzenadj_static)
    target_link_libraries(ryzenadj_shared)
    
    # Add Windows definitions
    target_compile_definitions(ryzenadj_static PRIVATE _WIN32)
    target_compile_definitions(ryzenadj_shared PRIVATE _WIN32)
else()
    # Linux specific libraries
    target_link_libraries(ryzenadj_static m)
    target_link_libraries(ryzenadj_shared m)
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ryzenadj_static PRIVATE -Wall -Wextra -O2)
    target_compile_options(ryzenadj_shared PRIVATE -Wall -Wextra -O2)
endif()

# Installation
install(TARGETS ryzenadj_static ryzenadj_shared
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${RYZENADJ_HEADERS} DESTINATION include)

# Example executable (optional)
option(BUILD_EXAMPLES "Build example programs" OFF)

if(BUILD_EXAMPLES)
    # Simple test program
    add_executable(ryzenadj_test test.c)
    target_link_libraries(ryzenadj_test ryzenadj_static)
    
    if(NOT WIN32)
        target_link_libraries(ryzenadj_test m)
    endif()
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "ryzenadj_standalone")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Standalone RyzenAdj library for HX370")
set(CPACK_PACKAGE_VENDOR "RyzenAdj")

include(CPack)