#pragma once
#include <DirectXMath.h>
#include <algorithm>
using namespace DirectX;

inline float Clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }
inline float Smooth01(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }
inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline XMFLOAT3 Sub(const XMFLOAT3& a, const XMFLOAT3& b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline XMFLOAT3 Mul(const XMFLOAT3& a, float s) { return {a.x*s,a.y*s,a.z*s}; }
inline float Dot3(const XMFLOAT3& a, const XMFLOAT3& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline XMFLOAT3 Normalize3(const XMFLOAT3& v) { XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&v)); XMFLOAT3 r; XMStoreFloat3(&r,n); return r; }
