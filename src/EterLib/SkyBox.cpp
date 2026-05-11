#include "stdafx.h"
#include "SkyBox.h"
#include "Camera.h"
#include "StateManager.h"
#include "ResourceManager.h"

#include "EterBase/Timer.h"
#include "qMin32Lib/all.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cmath>

CSkyObjectQuad::CSkyObjectQuad()
{
	m_Indices = { 0, 2, 1, 3 };
	for (auto& vertex : m_Vertex)
		std::memset(&vertex, 0, sizeof(TPDTVertex));
}

void CSkyObjectQuad::Clear(unsigned char vertexIndex, float r, float g, float b, float a)
{
	if (vertexIndex >= m_Helper.size())
		return;
	m_Helper[vertexIndex].Clear(r, g, b, a);
}

void CSkyObjectQuad::SetSrcColor(unsigned char vertexIndex, float r, float g, float b, float a)
{
	if (vertexIndex >= m_Helper.size())
		return;
	m_Helper[vertexIndex].SetSrcColor(r, g, b, a);
}

void CSkyObjectQuad::SetTransition(unsigned char vertexIndex, float r, float g, float b, float a, DWORD duration)
{
	if (vertexIndex >= m_Helper.size())
		return;
	m_Helper[vertexIndex].SetTransition(r, g, b, a, duration);
}

void CSkyObjectQuad::SetVertex(unsigned char vertexIndex, const TPDTVertex& vertex)
{
	if (vertexIndex >= m_Vertex.size())
		return;
	m_Vertex[m_Indices[vertexIndex]] = vertex;
}

void CSkyObjectQuad::StartTransition()
{
	for (auto& helper : m_Helper)
		helper.StartTransition();
}

bool CSkyObjectQuad::Update()
{
	bool changed = false;
	for (size_t i = 0; i < m_Helper.size(); ++i)
	{
		changed = m_Helper[i].Update() || changed;
		m_Vertex[m_Indices[i]].diffuse = m_Helper[i].GetCurColor();
	}
	return changed;
}

void CSkyObjectQuad::Render()
{
	if (CGraphicBase::SetPDTStream(m_Vertex.data(), static_cast<UINT>(m_Vertex.size())))
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, 0);
}

CSkyObject::CSkyObject()
{
	D3DXMatrixIdentity(&m_matWorld);
	D3DXMatrixIdentity(&m_matTranslation);
	D3DXMatrixIdentity(&m_matWorldCloud);
	D3DXMatrixIdentity(&m_matTextureCloud);
	m_dwlastTime = CTimer::Instance().GetCurrentMillisecond();
}

void CSkyObject::Update()
{
	CCamera* camera = CCameraManager::Instance().GetCurrentCamera();
	if (!camera)
		return;

	RenderFrameContext ctx = RenderFrameContext::Default();
	ctx.Eye = camera->GetEye();
	Update(ctx);
}

void CSkyObject::Update(const RenderFrameContext& ctx)
{
	const D3DXVECTOR3 eye = ctx.Eye;
	if (m_v3Position == eye && !m_bSkyMatrixUpdated)
		return;

	m_v3Position = eye;
	m_matWorld._41 = eye.x;
	m_matWorld._42 = eye.y;
	m_matWorld._43 = eye.z;
	m_matWorldCloud._41 = eye.x;
	m_matWorldCloud._42 = eye.y;
	m_matWorldCloud._43 = eye.z + m_fCloudHeight;
	m_bSkyMatrixUpdated = false;
}

void CSkyObject::StartTransition()
{
	m_bTransitionStarted = true;
}

CGraphicImageInstance* CSkyObject::GenerateTexture(const char* filename)
{
	if (!filename || !filename[0])
		return nullptr;

	CResource* resource = CResourceManager::Instance().GetResourcePointer(filename);
	if (!resource || !resource->IsType(CGraphicImage::Type()))
		return nullptr;

	CGraphicImageInstance* imageInstance = CGraphicImageInstance::New();
	imageInstance->SetImagePointer(static_cast<CGraphicImage*>(resource));
	return imageInstance;
}

void CSkyObject::DeleteTexture(CGraphicImageInstance* imageInstance)
{
	if (imageInstance)
		CGraphicImageInstance::Delete(imageInstance);
}

void CSkyObject::TSkyObjectFace::StartTransition()
{
	for (auto& quad : m_SkyObjectQuadVector)
		quad.StartTransition();
}

bool CSkyObject::TSkyObjectFace::Update()
{
	bool changed = false;
	for (auto& quad : m_SkyObjectQuadVector)
		changed = quad.Update() || changed;
	return changed;
}

void CSkyObject::TSkyObjectFace::Render()
{
	for (auto& quad : m_SkyObjectQuadVector)
		quad.Render();
}

CSkyBox::CSkyBox() = default;

CSkyBox::~CSkyBox()
{
	Destroy();
}

void CSkyBox::Destroy()
{
	Unload();
	for (auto& face : m_Faces)
		face.m_SkyObjectQuadVector.clear();
	m_FaceCloud.m_SkyObjectQuadVector.clear();
}

void CSkyBox::Unload()
{
	for (auto& [_, imageInstance] : m_GraphicImageInstanceMap)
		DeleteTexture(imageInstance);
	m_GraphicImageInstanceMap.clear();
	for (auto& face : m_Faces)
		face.m_strFaceTextureFileName.clear();
	m_FaceCloud.m_strfacename.clear();
}

void CSkyBox::SetSkyBoxScale(const D3DXVECTOR3& scale)
{
	m_fScaleX = scale.x;
	m_fScaleY = scale.y;
	m_fScaleZ = scale.z;
	D3DXMatrixScaling(&m_matWorld, m_fScaleX, m_fScaleY, m_fScaleZ);
	m_bSkyMatrixUpdated = true;
}

void CSkyBox::SetGradientLevel(BYTE upper, BYTE lower)
{
	m_ucVirticalGradientLevelUpper = upper;
	m_ucVirticalGradientLevelLower = lower;
}

void CSkyBox::SetFaceTexture(const char* filename, int faceIndex)
{
	if (!filename || !filename[0] || faceIndex < 0 || faceIndex >= static_cast<int>(m_Faces.size()))
		return;

	m_Faces[faceIndex].m_strFaceTextureFileName = filename;
	if (m_GraphicImageInstanceMap.find(filename) != m_GraphicImageInstanceMap.end())
		return;

	CGraphicImageInstance* imageInstance = GenerateTexture(filename);
	if (!imageInstance)
	{
		TraceError("CSkyBox::SetFaceTexture - failed to load %s", filename);
		return;
	}
	m_GraphicImageInstanceMap.emplace(filename, imageInstance);
}

void CSkyBox::SetCloudTexture(const char* filename)
{
	if (!filename || !filename[0])
		return;

	m_FaceCloud.m_strfacename = filename;
	if (m_GraphicImageInstanceMap.find(filename) != m_GraphicImageInstanceMap.end())
		return;

	CGraphicImageInstance* imageInstance = GenerateTexture(filename);
	if (!imageInstance)
	{
		TraceError("CSkyBox::SetCloudTexture - failed to load %s", filename);
		return;
	}
	m_GraphicImageInstanceMap.emplace(filename, imageInstance);
}

void CSkyBox::SetCloudScale(const D3DXVECTOR2& cloudScale)
{
	m_fCloudScaleX = cloudScale.x;
	m_fCloudScaleY = cloudScale.y;
	D3DXMatrixScaling(&m_matWorldCloud, m_fCloudScaleX, m_fCloudScaleY, 1.0f);
	m_bSkyMatrixUpdated = true;
}

void CSkyBox::SetCloudHeight(float height)
{
	m_fCloudHeight = height;
	m_bSkyMatrixUpdated = true;
}

void CSkyBox::SetCloudTextureScale(const D3DXVECTOR2& cloudTextureScale)
{
	m_fCloudTextureScaleX = cloudTextureScale.x;
	m_fCloudTextureScaleY = cloudTextureScale.y;
	m_matTextureCloud._11 = m_fCloudTextureScaleX;
	m_matTextureCloud._22 = m_fCloudTextureScaleY;
}

void CSkyBox::SetCloudScrollSpeed(const D3DXVECTOR2& cloudScrollSpeed)
{
	m_fCloudScrollSpeedU = cloudScrollSpeed.x;
	m_fCloudScrollSpeedV = cloudScrollSpeed.y;
}

void CSkyBox::SetSkyObjectQuadVertical(TSkyObjectQuadVector* quads, const D3DXVECTOR2* points)
{
	if (!quads || !points)
		return;

	const uint32_t upper = m_ucVirticalGradientLevelUpper;
	const uint32_t lower = m_ucVirticalGradientLevelLower;
	quads->clear();
	quads->resize(upper + lower);

	uint32_t index = 0;
	TPDTVertex vertex{};

	for (uint32_t y = 0; y < upper; ++y)
	{
		CSkyObjectQuad& quad = quads->at(index++);
		const float y0 = static_cast<float>(y) / static_cast<float>(upper);
		const float y1 = static_cast<float>(y + 1) / static_cast<float>(upper);
		vertex.position = D3DXVECTOR3(points[0].x, points[0].y, 1.0f - y1); vertex.texCoord = D3DXVECTOR2(0.0f, y1 * 0.5f); quad.SetVertex(0, vertex);
		vertex.position = D3DXVECTOR3(points[0].x, points[0].y, 1.0f - y0); vertex.texCoord = D3DXVECTOR2(0.0f, y0 * 0.5f); quad.SetVertex(1, vertex);
		vertex.position = D3DXVECTOR3(points[1].x, points[1].y, 1.0f - y1); vertex.texCoord = D3DXVECTOR2(1.0f, y1 * 0.5f); quad.SetVertex(2, vertex);
		vertex.position = D3DXVECTOR3(points[1].x, points[1].y, 1.0f - y0); vertex.texCoord = D3DXVECTOR2(1.0f, y0 * 0.5f); quad.SetVertex(3, vertex);
	}

	for (uint32_t y = 0; y < lower; ++y)
	{
		CSkyObjectQuad& quad = quads->at(index++);
		const float y0 = static_cast<float>(y) / static_cast<float>(lower);
		const float y1 = static_cast<float>(y + 1) / static_cast<float>(lower);
		vertex.position = D3DXVECTOR3(points[0].x, points[0].y, -y1); vertex.texCoord = D3DXVECTOR2(0.0f, 0.5f + y1 * 0.5f); quad.SetVertex(0, vertex);
		vertex.position = D3DXVECTOR3(points[0].x, points[0].y, -y0); vertex.texCoord = D3DXVECTOR2(0.0f, 0.5f + y0 * 0.5f); quad.SetVertex(1, vertex);
		vertex.position = D3DXVECTOR3(points[1].x, points[1].y, -y1); vertex.texCoord = D3DXVECTOR2(1.0f, 0.5f + y1 * 0.5f); quad.SetVertex(2, vertex);
		vertex.position = D3DXVECTOR3(points[1].x, points[1].y, -y0); vertex.texCoord = D3DXVECTOR2(1.0f, 0.5f + y0 * 0.5f); quad.SetVertex(3, vertex);
	}
}

void CSkyBox::SetSkyObjectQuadHorizon(TSkyObjectQuadVector* quads, const D3DXVECTOR3* points)
{
	if (!quads || !points)
		return;

	quads->clear();
	quads->resize(1);
	CSkyObjectQuad& quad = quads->front();
	TPDTVertex vertex{};
	vertex.position = points[0]; vertex.texCoord = D3DXVECTOR2(0.0f, 1.0f); quad.SetVertex(0, vertex);
	vertex.position = points[1]; vertex.texCoord = D3DXVECTOR2(0.0f, 0.0f); quad.SetVertex(1, vertex);
	vertex.position = points[2]; vertex.texCoord = D3DXVECTOR2(1.0f, 1.0f); quad.SetVertex(2, vertex);
	vertex.position = points[3]; vertex.texCoord = D3DXVECTOR2(1.0f, 0.0f); quad.SetVertex(3, vertex);
}

void CSkyBox::Refresh()
{
	D3DXVECTOR3 points3[4]{};
	D3DXVECTOR2 points2[2]{};

	if (m_ucRenderMode == SKY_RENDER_MODE_DEFAULT || m_ucRenderMode == SKY_RENDER_MODE_DIFFUSE)
	{
		if (m_ucVirticalGradientLevelUpper + m_ucVirticalGradientLevelLower == 0)
			return;

		points2[0] = D3DXVECTOR2(1.0f, -1.0f); points2[1] = D3DXVECTOR2(-1.0f, -1.0f); SetSkyObjectQuadVertical(&m_Faces[0].m_SkyObjectQuadVector, points2); m_Faces[0].m_strfacename = "front";
		points2[0] = D3DXVECTOR2(-1.0f, 1.0f); points2[1] = D3DXVECTOR2(1.0f, 1.0f); SetSkyObjectQuadVertical(&m_Faces[1].m_SkyObjectQuadVector, points2); m_Faces[1].m_strfacename = "back";
		points2[0] = D3DXVECTOR2(-1.0f, -1.0f); points2[1] = D3DXVECTOR2(-1.0f, 1.0f); SetSkyObjectQuadVertical(&m_Faces[2].m_SkyObjectQuadVector, points2); m_Faces[2].m_strfacename = "left";
		points2[0] = D3DXVECTOR2(1.0f, 1.0f); points2[1] = D3DXVECTOR2(1.0f, -1.0f); SetSkyObjectQuadVertical(&m_Faces[3].m_SkyObjectQuadVector, points2); m_Faces[3].m_strfacename = "right";
		points3[0] = D3DXVECTOR3(1.0f, 1.0f, 1.0f); points3[1] = D3DXVECTOR3(-1.0f, 1.0f, 1.0f); points3[2] = D3DXVECTOR3(1.0f, -1.0f, 1.0f); points3[3] = D3DXVECTOR3(-1.0f, -1.0f, 1.0f); SetSkyObjectQuadHorizon(&m_Faces[4].m_SkyObjectQuadVector, points3); m_Faces[4].m_strfacename = "top";
		points3[0] = D3DXVECTOR3(-1.0f, 1.0f, -1.0f); points3[1] = D3DXVECTOR3(1.0f, 1.0f, -1.0f); points3[2] = D3DXVECTOR3(-1.0f, -1.0f, -1.0f); points3[3] = D3DXVECTOR3(1.0f, -1.0f, -1.0f); SetSkyObjectQuadHorizon(&m_Faces[5].m_SkyObjectQuadVector, points3); m_Faces[5].m_strfacename = "bottom";
	}
	else if (m_ucRenderMode == SKY_RENDER_MODE_TEXTURE)
	{
		points3[0] = D3DXVECTOR3(1.0f, -1.0f, -1.0f); points3[1] = D3DXVECTOR3(1.0f, -1.0f, 1.0f); points3[2] = D3DXVECTOR3(-1.0f, -1.0f, -1.0f); points3[3] = D3DXVECTOR3(-1.0f, -1.0f, 1.0f); SetSkyObjectQuadHorizon(&m_Faces[0].m_SkyObjectQuadVector, points3); m_Faces[0].m_strfacename = "front";
		points3[0] = D3DXVECTOR3(-1.0f, 1.0f, -1.0f); points3[1] = D3DXVECTOR3(-1.0f, 1.0f, 1.0f); points3[2] = D3DXVECTOR3(1.0f, 1.0f, -1.0f); points3[3] = D3DXVECTOR3(1.0f, 1.0f, 1.0f); SetSkyObjectQuadHorizon(&m_Faces[1].m_SkyObjectQuadVector, points3); m_Faces[1].m_strfacename = "back";
		points3[0] = D3DXVECTOR3(1.0f, 1.0f, -1.0f); points3[1] = D3DXVECTOR3(1.0f, 1.0f, 1.0f); points3[2] = D3DXVECTOR3(1.0f, -1.0f, -1.0f); points3[3] = D3DXVECTOR3(1.0f, -1.0f, 1.0f); SetSkyObjectQuadHorizon(&m_Faces[2].m_SkyObjectQuadVector, points3); m_Faces[2].m_strfacename = "left";
		points3[0] = D3DXVECTOR3(-1.0f, -1.0f, -1.0f); points3[1] = D3DXVECTOR3(-1.0f, -1.0f, 1.0f); points3[2] = D3DXVECTOR3(-1.0f, 1.0f, -1.0f); points3[3] = D3DXVECTOR3(-1.0f, 1.0f, 1.0f); SetSkyObjectQuadHorizon(&m_Faces[3].m_SkyObjectQuadVector, points3); m_Faces[3].m_strfacename = "right";
		points3[0] = D3DXVECTOR3(1.0f, -1.0f, 1.0f); points3[1] = D3DXVECTOR3(1.0f, 1.0f, 1.0f); points3[2] = D3DXVECTOR3(-1.0f, -1.0f, 1.0f); points3[3] = D3DXVECTOR3(-1.0f, 1.0f, 1.0f); SetSkyObjectQuadHorizon(&m_Faces[4].m_SkyObjectQuadVector, points3); m_Faces[4].m_strfacename = "top";
		points3[0] = D3DXVECTOR3(1.0f, -1.0f, -1.0f); points3[1] = D3DXVECTOR3(1.0f, 1.0f, -1.0f); points3[2] = D3DXVECTOR3(-1.0f, -1.0f, -1.0f); points3[3] = D3DXVECTOR3(-1.0f, 1.0f, -1.0f); SetSkyObjectQuadHorizon(&m_Faces[5].m_SkyObjectQuadVector, points3); m_Faces[5].m_strfacename = "bottom";
	}

	points3[0] = D3DXVECTOR3(1.0f, 1.0f, 0.0f);
	points3[1] = D3DXVECTOR3(-1.0f, 1.0f, 0.0f);
	points3[2] = D3DXVECTOR3(1.0f, -1.0f, 0.0f);
	points3[3] = D3DXVECTOR3(-1.0f, -1.0f, 0.0f);
	SetSkyObjectQuadHorizon(&m_FaceCloud.m_SkyObjectQuadVector, points3);
}

void CSkyBox::SetQuadColor(CSkyObjectQuad& quad, unsigned char vertexIndex, const TColor& color, const TColor& nextColor, DWORD transitionTime)
{
	quad.SetSrcColor(vertexIndex, color.r, color.g, color.b, color.a);
	quad.SetTransition(vertexIndex, nextColor.r, nextColor.g, nextColor.b, nextColor.a, transitionTime);
}

void CSkyBox::SetCloudColor(const TGradientColor& color, const TGradientColor& nextColor, const DWORD& transitionTime)
{
	for (auto& quad : m_FaceCloud.m_SkyObjectQuadVector)
		for (unsigned char i = 0; i < 4; ++i)
			SetQuadColor(quad, i, color.m_FirstColor, nextColor.m_FirstColor, transitionTime);
}

void CSkyBox::SetSkyColor(const TVectorGradientColor& colorVector, const TVectorGradientColor& nextColorVector, long transitionTime)
{
	if (colorVector.empty() || nextColorVector.empty())
		return;

	const size_t count = std::min(colorVector.size(), nextColorVector.size());
	const DWORD duration = static_cast<DWORD>(std::max<long>(transitionTime, 0));

	for (size_t faceIndex = 0; faceIndex < 4; ++faceIndex)
	{
		size_t colorIndex = 0;
		for (auto& quad : m_Faces[faceIndex].m_SkyObjectQuadVector)
		{
			const size_t idx = std::min(colorIndex++, count - 1);
			SetQuadColor(quad, 0, colorVector[idx].m_SecondColor, nextColorVector[idx].m_SecondColor, duration);
			SetQuadColor(quad, 1, colorVector[idx].m_FirstColor, nextColorVector[idx].m_FirstColor, duration);
			SetQuadColor(quad, 2, colorVector[idx].m_SecondColor, nextColorVector[idx].m_SecondColor, duration);
			SetQuadColor(quad, 3, colorVector[idx].m_FirstColor, nextColorVector[idx].m_FirstColor, duration);
		}
	}

	for (auto& quad : m_Faces[4].m_SkyObjectQuadVector)
		for (unsigned char i = 0; i < 4; ++i)
			SetQuadColor(quad, i, colorVector.front().m_FirstColor, nextColorVector.front().m_FirstColor, duration);

	for (auto& quad : m_Faces[5].m_SkyObjectQuadVector)
		for (unsigned char i = 0; i < 4; ++i)
			SetQuadColor(quad, i, colorVector.back().m_SecondColor, nextColorVector.back().m_SecondColor, duration);
}

void CSkyBox::StartTransition()
{
	CSkyObject::StartTransition();
	for (auto& face : m_Faces)
		face.StartTransition();
	m_FaceCloud.StartTransition();
}

void CSkyBox::Update()
{
	CSkyObject::Update();
	for (auto& face : m_Faces)
		face.Update();
	m_FaceCloud.Update();
}

void CSkyBox::Update(const RenderFrameContext& ctx)
{
	CSkyObject::Update(ctx);
	for (auto& face : m_Faces)
		face.Update();
	m_FaceCloud.Update();
}

CGraphicImageInstance* CSkyBox::FindTexture(const std::string& filename) const
{
	const auto it = m_GraphicImageInstanceMap.find(filename);
	return it == m_GraphicImageInstanceMap.end() ? nullptr : it->second;
}

void CSkyBox::Render()
{
	Render(RenderFrameContext::Default());
}

void CSkyBox::Render(const RenderFrameContext& ctx)
{
	Update(ctx);

	auto& state = STATEMANAGER.GetStateCache();
	state.Push();

	state.DepthStencil.SetDepthEnable(TRUE);
	state.DepthStencil.SetDepthWriteEnable(FALSE);
	state.Blend.SetBlendEnable(FALSE);

	_mgr->GetCbMgr()->SetLightingEnable(FALSE);
	_mgr->GetCbMgr()->SetFogEnable(FALSE);

	STATEMANAGER.SetTexture(1, NULL);
	STATEMANAGER.GetTransform().SetWorld(m_matWorld);
	STATEMANAGER.GetTransform().SetView(ctx.View);
	STATEMANAGER.GetTransform().SetProjection(ctx.Projection);

	if (m_ucRenderMode == SKY_RENDER_MODE_TEXTURE)
	{
		_mgr->SetShader(VF_SKYBOX, SKY_USE_TEXTURE);
		state.Sampler.Push(0);
		state.Sampler.SetAddressUV(0, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
		for (auto& face : m_Faces)
		{
			CGraphicImageInstance* imageInstance = FindTexture(face.m_strFaceTextureFileName);
			if (!imageInstance || !imageInstance->GetTextureReference().GetSRV())
				continue;
			STATEMANAGER.SetTexture(0, imageInstance->GetTextureReference().GetSRV());
			face.Render();
		}
		state.Sampler.Restore(0);
	}
	else
	{
		_mgr->SetShader(VF_SKYBOX, SKY_USE_DIFFUSE);
		for (auto& face : m_Faces)
			face.Render();
	}

	STATEMANAGER.SetTexture(0, NULL);
	state.Restore();
}

void CSkyBox::RenderCloud()
{
	RenderCloud(RenderFrameContext::Default());
}

void CSkyBox::RenderCloud(const RenderFrameContext& ctx)
{
	if (m_FaceCloud.m_strfacename.empty())
		return;

	CGraphicImageInstance* imageInstance = FindTexture(m_FaceCloud.m_strfacename);
	if (!imageInstance || !imageInstance->GetTexturePointer())
		return;

	Update(ctx);

	auto& state = STATEMANAGER.GetStateCache();
	state.Push();

	state.DepthStencil.SetDepthEnable(TRUE);
	state.DepthStencil.SetDepthWriteEnable(FALSE);
	state.Blend.SetBlendEnable(TRUE);
	state.Blend.SetSrcBlend(D3D11_BLEND_ONE);
	state.Blend.SetDestBlend(D3D11_BLEND_INV_SRC_COLOR);

	_mgr->GetCbMgr()->SetLightingEnable(FALSE);
	_mgr->GetCbMgr()->SetFogEnable(FALSE);

	const float delta = ctx.DeltaTime > 0.0f ? ctx.DeltaTime : 0.0f;
	m_fCloudPositionU += m_fCloudScrollSpeedU * delta;
	m_fCloudPositionV += m_fCloudScrollSpeedV * delta;
	m_fCloudPositionU -= std::floor(m_fCloudPositionU);
	m_fCloudPositionV -= std::floor(m_fCloudPositionV);
	m_matTextureCloud._31 = m_fCloudPositionU;
	m_matTextureCloud._32 = m_fCloudPositionV;

	_mgr->SetShader(VF_SKYBOX, SKY_CLOUD);
	STATEMANAGER.GetTransform().SetWorld(m_matWorldCloud);
	STATEMANAGER.GetTransform().SetTexture0(m_matTextureCloud);
	STATEMANAGER.GetTransform().SetView(ctx.View);
	STATEMANAGER.GetTransform().SetProjection(ctx.Projection);
	STATEMANAGER.SetTexture(0, imageInstance->GetTexturePointer()->GetSRV());
	m_FaceCloud.Render();
	STATEMANAGER.SetTexture(0, NULL);

	state.Restore();
}
