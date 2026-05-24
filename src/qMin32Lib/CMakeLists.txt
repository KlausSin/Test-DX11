file(GLOB_RECURSE FILE_SOURCES CONFIGURE_DEPENDS
    "*.h"
    "*.cpp"
)

add_library(qMin32Lib STATIC ${FILE_SOURCES})

target_include_directories(qMin32Lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_definitions(qMin32Lib PRIVATE
    JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
    JPH_USE_DX12
    JPH_USE_CPU_COMPUTE
    JPH_DEBUG_RENDERER
    JPH_PROFILE_ENABLED
    JPH_OBJECT_STREAM
    JPH_USE_AVX2
    JPH_USE_AVX
    JPH_USE_SSE4_1
    JPH_USE_SSE4_2
    JPH_USE_LZCNT
    JPH_USE_TZCNT
    JPH_USE_F16C
    JPH_USE_FMADD
)

target_link_libraries(qMin32Lib PRIVATE
    ${EXTERN_LIBS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${FILE_SOURCES})

source_group("assets" REGULAR_EXPRESSION "assets/.*")
source_group("jolt" REGULAR_EXPRESSION "jolt/.*")
source_group("state" REGULAR_EXPRESSION "state/.*")
source_group("entity" REGULAR_EXPRESSION "entity/.*")
