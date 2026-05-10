#pragma once
#include <d3d11.h>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cstddef>

#ifndef SAFE_RELEASE_D3D11
#define SAFE_RELEASE_D3D11(p) do { if (p) { (p)->Release(); (p) = nullptr; } } while (0)
#endif

struct SD3D11RawHash
{
	template <class T>
	size_t operator()(const T& key) const
	{
		size_t h = 1469598103934665603ull;
		const unsigned char* p = reinterpret_cast<const unsigned char*>(&key);
		for (size_t i = 0; i < sizeof(T); ++i)
		{
			h ^= p[i];
			h *= 1099511628211ull;
		}
		return h;
	}
};
