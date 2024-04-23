#
# File: CMakeLists.txt
#
# Main cmakelists for the trackerboy repo.
# See README.md for details on building
#

cmake_minimum_required(VERSION 3.12...3.21)

# removes default /W3 flag for MSVC 
# https://stackoverflow.com/questions/45995784/how-to-set-compiler-options-with-cmake-in-visual-studio-2017
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/CompilerOverrides.cmake")

project (
    trackerboy 
    VERSION "0.6.5"
    LANGUAGES C CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake ${CMAKE_MODULE_PATH})

option(ENABLE_UNITY "Enable unity builds" OFF)
option(BUILD_TESTING "Build unit tests" OFF)

if (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(BUILD_ARCH "x86")
    set(BUILD_IS_32BIT TRUE)
else ()
    set(BUILD_ARCH "x64")
    set(BUILD_IS_32BIT FALSE)
endif ()

# assume Debug if CMAKE_BUILD_TYPE was not provided
if (NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE MATCHES "Debug")
    set(DEBUG_BUILD TRUE)
else ()
    set(DEBUG_BUILD FALSE)
endif ()

# enable LTO for non-debug builds
if (NOT DEBUG_BUILD)
    message(CHECK_START "checking if LTO is supported")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipoSupported OUTPUT output)
    if (ipoSupported)
        message(CHECK_PASS "yes")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else ()
        message(CHECK_FAIL "no")
    endif ()
endif()

# vendored code
add_subdirectory(vendored EXCLUDE_FROM_ALL)

#
# Main program
#
add_subdirectory(src)

#
# Unit tests
#
if (BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
endif ()

message(
    "\n"
    "Configuration summary\n"
    "---------------------\n"
    " * Install directory           : ${CMAKE_INSTALL_PREFIX}\n"
    " * Build type                  : ${CMAKE_BUILD_TYPE}\n"
    " * Architecture                : ${BUILD_ARCH}\n"
    " * Tests                       : ${BUILD_TESTING}\n"
    " * Unity build                 : ${ENABLE_UNITY}\n"
)
