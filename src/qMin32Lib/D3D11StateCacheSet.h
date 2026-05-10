#pragma once
#include "D3D11SamplerStateCache.h"
#include "D3D11RasterStateCache.h"
#include "D3D11DepthStencilStateCache.h"
#include "D3D11BlendStateCache.h"
#include "D3D11TransformStateCache.h"
#include "D3D11LightingStateCache.h"

class CD3D11StateCacheSet
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		Sampler.Initialize(device, context);
		Raster.Initialize(device, context);
		DepthStencil.Initialize(device, context);
		Blend.Initialize(device, context);
	}

	void Destroy()
	{
		Sampler.Destroy();
		Raster.Destroy();
		DepthStencil.Destroy();
		Blend.Destroy();
		Transform.ClearStack();
		Light.ClearStack();
	}

	void ResetDefault()
	{
		Sampler.ResetDefault();
		Raster.ResetDefault();
		DepthStencil.ResetDefault();
		Blend.ResetDefault();
		Transform.ResetDefault();
		Light.ResetDefault();
	}

	void ForceDirty()
	{
		Sampler.ForceDirty();
		Raster.ForceDirty();
		DepthStencil.ForceDirty();
		Blend.ForceDirty();
		Transform.ForceDirty();
		Light.ForceDirty();
	}

	void Push()
	{
		Sampler.Push();
		Raster.Push();
		DepthStencil.Push();
		Blend.Push();
		Transform.Push();
		Light.Push();
	}

	bool Restore()
	{
		bool ok = true;
		ok = Sampler.Restore() && ok;
		ok = Raster.Restore() && ok;
		ok = DepthStencil.Restore() && ok;
		ok = Blend.Restore() && ok;
		ok = Transform.Restore() && ok;
		ok = Light.Restore() && ok;
		return ok;
	}

	void ClearStacks()
	{
		Sampler.ClearStack();
		Raster.ClearStack();
		DepthStencil.ClearStack();
		Blend.ClearStack();
		Transform.ClearStack();
		Light.ClearStack();
	}

	void Apply()
	{
		Sampler.ApplyAllPS();
		Raster.Apply();
		DepthStencil.Apply();
		Blend.Apply();
	}

	void ApplyAllStages()
	{
		Sampler.ApplyAllStages();
		Raster.Apply();
		DepthStencil.Apply();
		Blend.Apply();
	}

	CD3D11SamplerStateCache Sampler;
	CD3D11RasterStateCache Raster;
	CD3D11DepthStencilStateCache DepthStencil;
	CD3D11BlendStateCache Blend;
	CD3D11TransformStateCache Transform;
	CD3D11LightingStateCache Light;
};
