file(GLOB_RECURSE FILE_SOURCES CONFIGURE_DEPENDS
    "*.h"
    "*.cpp"
)

set(ASSETS_SOURCES
    AssetTypeRegistry.h
    AssetTypes.h
    AsyncFileSystem.h
    IAsset.h
)

set(JOLT_SOURCES
    CPhysicsManager.h

    JoltTypes.h
    JoltConversion.h
    JoltPhysicsLayers.h
    JoltShapeCache.h
    JoltPhysicsConfig.h

    JoltPhysicsWorld.h
    JoltPhysicsWorld.cpp

    JoltPhysicsComponent.h
    JoltPhysicsComponent.cpp

    PhysicsObjectComponent.h

    CollisionTypes.h
    CollisionObject.h
    CollisionObject.cpp

    CharacterMotor.h
    CharacterMotor.cpp

    GameRulesAdapter.h
    GameRulesAdapter.cpp

    JoltCollisionStreamer.h
    JoltCollisionStreamer.cpp
)

set(STATE_SOURCES
    D3D11BlendStateCache.h
    D3D11BlendStateCache.cpp

    D3D11DepthStencilStateCache.h
    D3D11DepthStencilStateCache.cpp

    D3D11LightingStateCache.h
    D3D11LightingStateCache.cpp

    D3D11RasterStateCache.h
    D3D11RasterStateCache.cpp

    D3D11SamplerStateCache.h
    D3D11SamplerStateCache.cpp

    D3D11TransformStateCache.h
    D3D11TransformStateCache.cpp

    D3D11StateCacheSet.h
    D3D11StateCommon.h
)

add_library(qMin32Lib STATIC ${FILE_SOURCES})

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

set(FILTERED_SOURCES
    ${ASSETS_SOURCES}
    ${JOLT_SOURCES}
    ${STATE_SOURCES}
)

set(ROOT_SOURCES ${FILE_SOURCES})
list(REMOVE_ITEM ROOT_SOURCES ${FILTERED_SOURCES})

source_group("" FILES ${ROOT_SOURCES})
source_group("jolt" FILES ${JOLT_SOURCES})
source_group("state" FILES ${STATE_SOURCES})
source_group("assets" FILES ${ASSETS_SOURCES})
