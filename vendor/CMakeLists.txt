cmake_minimum_required(VERSION 3.19)

project(ExternalBuilder)

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(OUTPUT_ROOT ${ROOT_DIR}/../extern)
set(OUTPUT_INCLUDE_DIR ${OUTPUT_ROOT}/include)
set(OUTPUT_LIBRARY_DIR ${OUTPUT_ROOT}/library)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_LIBRARY_DIR}/debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_LIBRARY_DIR}/release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_LIBRARY_DIR}/release)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_LIBRARY_DIR}/debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_LIBRARY_DIR}/release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_LIBRARY_DIR}/release)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_LIBRARY_DIR}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_LIBRARY_DIR}/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_LIBRARY_DIR}/release)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/MP>")

add_definitions(-DNOMINMAX)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

add_subdirectory(lzo-2.10)
add_subdirectory(mio)
add_subdirectory(libsodium)

set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_ENABLE_ASM OFF CACHE BOOL "" FORCE)
add_subdirectory(zstd-1.5.7/build/cmake zstd)

if(EXISTS "${ROOT_DIR}/DirectXTex/CMakeLists.txt")
    add_subdirectory(DirectXTex)
endif()

if(EXISTS "${ROOT_DIR}/JoltPhysics/Build/CMakeLists.txt")
    add_subdirectory(JoltPhysics/Build JoltPhysics)
endif()

if(EXISTS "${ROOT_DIR}/SpeedTree/CMakeLists.txt")
    add_subdirectory(SpeedTree)
endif()


add_custom_target(copy_external_headers ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_INCLUDE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ROOT_DIR}/lzo-2.10/include" "${OUTPUT_INCLUDE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ROOT_DIR}/mio/include" "${OUTPUT_INCLUDE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ROOT_DIR}/libsodium/src/libsodium/include" "${OUTPUT_INCLUDE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ROOT_DIR}/zstd-1.5.7/lib" "${OUTPUT_INCLUDE_DIR}/zstd"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ROOT_DIR}/DirectXTex" "${OUTPUT_INCLUDE_DIR}/DirectXTex"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ROOT_DIR}/JoltPhysics/Jolt" "${OUTPUT_INCLUDE_DIR}/Jolt"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ROOT_DIR}/SpeedTree" "${OUTPUT_INCLUDE_DIR}/SpeedTree"
)

if(WIN32)
    if(TARGET lzo2)
        set_target_properties(lzo2 PROPERTIES FOLDER vendor)
    endif()

    if(TARGET sodium)
        set_target_properties(sodium PROPERTIES FOLDER vendor)
    endif()

    if(TARGET zstd)
        set_target_properties(zstd PROPERTIES FOLDER vendor/zstd)
    endif()

    if(TARGET libzstd_static)
        set_target_properties(libzstd_static PROPERTIES FOLDER vendor/zstd)
    endif()

    if(TARGET DirectXTex)
        set_target_properties(DirectXTex PROPERTIES FOLDER vendor/DirectXTex)
    endif()

    if(TARGET Jolt)
        set_target_properties(Jolt PROPERTIES FOLDER vendor/JoltPhysics)
    endif()

    if(TARGET SpeedTree)
        set_target_properties(SpeedTree PROPERTIES FOLDER vendor/SpeedTree)
    endif()
endif()