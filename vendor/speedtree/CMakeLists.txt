cmake_minimum_required(VERSION 3.19)

project(SpeedTree)

file(GLOB SPEEDTREE_ROOT CONFIGURE_DEPENDS
    "*.h"
    "*.hpp"
    "*.c"
    "*.cpp"
)

file(GLOB LIBFILENAME CONFIGURE_DEPENDS
    "LibFilename_Source/*.h"
    "LibFilename_Source/*.hpp"
    "LibFilename_Source/*.c"
    "LibFilename_Source/*.cpp"
)

file(GLOB LIBGLOBALS CONFIGURE_DEPENDS
    "LibGlobals_Source/*.h"
    "LibGlobals_Source/*.hpp"
    "LibGlobals_Source/*.c"
    "LibGlobals_Source/*.cpp"
)

file(GLOB LIBRANDOM CONFIGURE_DEPENDS
    "LibRandom_Source/*.h"
    "LibRandom_Source/*.hpp"
    "LibRandom_Source/*.c"
    "LibRandom_Source/*.cpp"
)

file(GLOB LIBSPLINE CONFIGURE_DEPENDS
    "LibSpline_Source/*.h"
    "LibSpline_Source/*.hpp"
    "LibSpline_Source/*.c"
    "LibSpline_Source/*.cpp"
)

file(GLOB LIBVECTOR CONFIGURE_DEPENDS
    "LibVector_Source/*.h"
    "LibVector_Source/*.hpp"
    "LibVector_Source/*.c"
    "LibVector_Source/*.cpp"
)

add_library(SpeedTree STATIC
    ${SPEEDTREE_ROOT}
    ${LIBFILENAME}
    ${LIBGLOBALS}
    ${LIBRANDOM}
    ${LIBSPLINE}
    ${LIBVECTOR}
)

target_include_directories(SpeedTree PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/LibFilename_Source
    ${CMAKE_CURRENT_LIST_DIR}/LibGlobals_Source
    ${CMAKE_CURRENT_LIST_DIR}/LibRandom_Source
    ${CMAKE_CURRENT_LIST_DIR}/LibSpline_Source
    ${CMAKE_CURRENT_LIST_DIR}/LibVector_Source
)

target_compile_definitions(SpeedTree PRIVATE
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    FULL_SPEEDTREE_LIB
)

target_compile_options(SpeedTree PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
    "$<$<CXX_COMPILER_ID:MSVC>:/MP>"
    "$<$<CXX_COMPILER_ID:MSVC>:/permissive>"
)

set_target_properties(SpeedTree PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)