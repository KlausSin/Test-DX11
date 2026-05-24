#pragma once
#include <cstdint>

struct JoltPhysicsConfig
{
    float streamCreateDistance = 4500.0f;
    float streamDestroyDistance = 6500.0f;
    float queryCreateDistance = 3500.0f;

    uint32_t maxBodyCreatesPerFrame = 8;
    uint32_t maxBodyDestroysPerFrame = 16;
    uint32_t maxImmediateQueryCreatesPerFrame = 2;

    float defaultPlaneThickness = 4.0f;
    float treeRadiusScale = 0.45f;
    float treeHeightScale = 0.80f;
};
