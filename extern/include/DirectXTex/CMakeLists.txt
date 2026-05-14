cmake_minimum_required(VERSION 3.19)

project(DirectXTex)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(
    -DUNICODE
    -D_UNICODE
    -DWIN32
    -DWIN32_LEAN_AND_MEAN
    -DNOMINMAX
    -D_CRT_SECURE_NO_WARNINGS
    -D_WIN32_WINNT=0x0A00
    -D_CRT_STDIO_ARBITRARY_WIDE_SPECIFIERS
)

file(GLOB DIRECTXTEX_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
    "*.inl"
)

list(FILTER DIRECTXTEX_SOURCES EXCLUDE REGEX ".*DirectXTexCompressGPU.*")
list(FILTER DIRECTXTEX_SOURCES EXCLUDE REGEX ".*BCDirectCompute.*")
list(FILTER DIRECTXTEX_SOURCES EXCLUDE REGEX ".*Texassemble.*")
list(FILTER DIRECTXTEX_SOURCES EXCLUDE REGEX ".*Texconv.*")
list(FILTER DIRECTXTEX_SOURCES EXCLUDE REGEX ".*Texdiag.*")
list(FILTER DIRECTXTEX_SOURCES EXCLUDE REGEX ".*DDSView.*")
list(FILTER DIRECTXTEX_SOURCES EXCLUDE REGEX ".*wicview.*")

add_library(DirectXTex STATIC
    ${DIRECTXTEX_SOURCES}
)

target_include_directories(DirectXTex PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(DirectXTex PUBLIC
    d3d11
    dxgi
    windowscodecs
)

target_compile_options(DirectXTex PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
    "$<$<CXX_COMPILER_ID:MSVC>:/MP>"
    "$<$<CXX_COMPILER_ID:MSVC>:/permissive>"
)

set_target_properties(DirectXTex PROPERTIES
    OUTPUT_NAME "DirectXTex"
)