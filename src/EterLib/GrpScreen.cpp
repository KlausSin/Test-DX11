#include "StdAfx.h"
#include "GrpScreen.h"
#include "Camera.h"
#include "StateManager.h"
#include "GrpDevice.h"

#include <comdef.h>
#include <utf8.h>

DWORD		CScreen::ms_diffuseColor = 0xffffffff;
DWORD		CScreen::ms_clearColor = 0L;
DWORD		CScreen::ms_clearStencil = 0L;
float		CScreen::ms_clearDepth = 1.0f;
Frustum		CScreen::ms_frustum;

extern bool GRAPHICS_CAPS_CAN_NOT_DRAW_LINE;

void CScreen::RenderLine3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	if (GRAPHICS_CAPS_CAN_NOT_DRAW_LINE)
		return;

	assert(ms_lpd3d11Device != NULL);

	TPDTVertex vertices[2] =
	{
		{ sx, sy, sz, ms_diffuseColor, 0.0f, 0.0f },
		{ ex, ey, ez, ms_diffuseColor, 0.0f, 0.0f }
	};
	
	// 2004.11.18.myevan.DrawIndexPrimitiveUP -> DynamicVertexBuffer
	if (SetPDTStream(vertices, 2))
	{	
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_LINELIST, 1, 0);
	}
}

void CScreen::RenderBox3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	if (GRAPHICS_CAPS_CAN_NOT_DRAW_LINE)
		return;

	assert(ms_lpd3d11Device != NULL);

	TPDTVertex vertices[8] =
	{
		{ sx, sy, sz, ms_diffuseColor, 0.0f, 0.0f },	// 0
		{ ex, sy, sz, ms_diffuseColor, 0.0f, 0.0f },	// 1

		{ sx, sy, sz, ms_diffuseColor, 0.0f, 0.0f },	// 0
		{ sx, ey, ez, ms_diffuseColor, 0.0f, 0.0f },	// 2

		{ ex, sy, sz, ms_diffuseColor, 0.0f, 0.0f },	// 1
		{ ex, ey, ez, ms_diffuseColor, 0.0f, 0.0f },	// 3

		{ sx, ey, ez, ms_diffuseColor, 0.0f, 0.0f },	// 2
		{ ex+1.0f, ey, ez, ms_diffuseColor, 0.0f, 0.0f }	// 3, (x가 1증가된 3)
	};

	// 2004.11.18.myevan.DrawIndexPrimitiveUP -> DynamicVertexBuffer
	if (SetPDTStream(vertices, 8))
	{
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_LINELIST, 4, 0);
	}
}

void CScreen::RenderBar3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	assert(ms_lpd3d11Device != NULL);

	TPDTVertex vertices[4] =
	{
		{ sx, sy, sz, ms_diffuseColor, 0.0f, 0.0f },
		{ sx, ey, ez, ms_diffuseColor, 0.0f, 0.0f },
		{ ex, sy, sz, ms_diffuseColor, 0.0f, 0.0f },
		{ ex, ey, ez, ms_diffuseColor, 0.0f, 0.0f },
	};

	

	if (SetPDTStream(vertices, 4))
	{
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, 0);
	}
}

void CScreen::RenderBar3d(const XMFLOAT3 * c_pv3Positions)
{
	assert(ms_lpd3d11Device != NULL);
	
	TPDTVertex vertices[4] =
	{
		{ c_pv3Positions[0].x, c_pv3Positions[0].y, c_pv3Positions[0].z, ms_diffuseColor, 0.0f, 0.0f },
		{ c_pv3Positions[2].x, c_pv3Positions[2].y, c_pv3Positions[2].z, ms_diffuseColor, 0.0f, 0.0f },
		{ c_pv3Positions[1].x, c_pv3Positions[1].y, c_pv3Positions[1].z, ms_diffuseColor, 0.0f, 0.0f },
		{ c_pv3Positions[3].x, c_pv3Positions[3].y, c_pv3Positions[3].z, ms_diffuseColor, 0.0f, 0.0f },
	};


	if (SetPDTStream(vertices, 4))
	{
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, 0);
	}
}

void CScreen::RenderGradationBar3d(float sx, float sy, float sz, float ex, float ey, float ez, DWORD dwStartColor, DWORD dwEndColor)
{
	assert(ms_lpd3d11Device != NULL);
	if (sx==ex) return;
	if (sy==ey) return;

	TPDTVertex vertices[4] =
	{
		{ sx, sy, sz, dwStartColor, 0.0f, 0.0f },
		{ sx, ey, ez, dwEndColor, 0.0f, 0.0f },
		{ ex, sy, sz, dwStartColor, 0.0f, 0.0f },
		{ ex, ey, ez, dwEndColor, 0.0f, 0.0f },
	};

	if (SetPDTStream(vertices, 4))
	{
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, 0);
	}
}

void CScreen::RenderLineCube(float sx, float sy, float sz, float ex, float ey, float ez)
{
	TPDTVertex vertices[8] =
	{
		{ sx, sy, sz, ms_diffuseColor, 0.0f, 0.0f },
		{ ex, sy, sz, ms_diffuseColor, 0.0f, 0.0f },
		{ sx, ey, sz, ms_diffuseColor, 0.0f, 0.0f },
		{ ex, ey, sz, ms_diffuseColor, 0.0f, 0.0f },
		{ sx, sy, ez, ms_diffuseColor, 0.0f, 0.0f },
		{ ex, sy, ez, ms_diffuseColor, 0.0f, 0.0f },
		{ sx, ey, ez, ms_diffuseColor, 0.0f, 0.0f },
		{ ex, ey, ez, ms_diffuseColor, 0.0f, 0.0f },
	};


	if (SetPDTStream(vertices, 8))
	{
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
		STATEMANAGER.GetTransform().SetWorld(ms_matStack.back());
		SetDefaultIndexBuffer(DEFAULT_IB_LINE_CUBE);

		STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_LINELIST, 0, 0, 4 * 3);
	}
}

void CScreen::RenderCube(float sx, float sy, float sz, float ex, float ey, float ez)
{
	TPDTVertex vertices[8] =
	{
		{ sx, sy, sz, ms_diffuseColor, 0.0f, 0.0f  },
		{ ex, sy, sz, ms_diffuseColor, 0.0f, 0.0f  },
		{ sx, ey, sz, ms_diffuseColor, 0.0f, 0.0f  },
		{ ex, ey, sz, ms_diffuseColor, 0.0f, 0.0f  },
		{ sx, sy, ez, ms_diffuseColor, 0.0f, 0.0f  },
		{ ex, sy, ez, ms_diffuseColor, 0.0f, 0.0f  },
		{ sx, ey, ez, ms_diffuseColor, 0.0f, 0.0f  },
		{ ex, ey, ez, ms_diffuseColor, 0.0f, 0.0f  },
	};
	

	if (SetPDTStream(vertices, 8))
	{
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
		STATEMANAGER.GetTransform().SetWorld(ms_matStack.back());

		SetDefaultIndexBuffer(DEFAULT_IB_FILL_CUBE);
		STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 0, 4 * 3);
	}
}

void CScreen::RenderCube(float sx, float sy, float sz, float ex, float ey, float ez, XMFLOAT4X4 matRotation)
{
	XMFLOAT3 v3Center = XMFLOAT3((sx + ex) * 0.5f, (sy + ey) * 0.5f, (sz + ez) * 0.5f);
	XMFLOAT3 v3Vertex[8] = 
	{
		XMFLOAT3(sx, sy, sz),
		XMFLOAT3(ex, sy, sz),
		XMFLOAT3(sx, ey, sz),
		XMFLOAT3(ex, ey, sz),
		XMFLOAT3(sx, sy, ez),
		XMFLOAT3(ex, sy, ez),
		XMFLOAT3(sx, ey, ez),
		XMFLOAT3(ex, ey, ez),
	};
	TPDTVertex vertices[8];

	for (int i = 0; i < 8; ++i)
	{
		XMFLOAT3 local =
		{
			v3Vertex[i].x - v3Center.x,
			v3Vertex[i].y - v3Center.y,
			v3Vertex[i].z - v3Center.z
		};

		XMStoreFloat3(&v3Vertex[i], XMVector3TransformCoord(XMLoadFloat3(&local), XMLoadFloat4x4(&matRotation)));

		v3Vertex[i].x += v3Center.x;
		v3Vertex[i].y += v3Center.y;
		v3Vertex[i].z += v3Center.z;

		vertices[i].position = v3Vertex[i];
		vertices[i].diffuse = ms_diffuseColor;
		vertices[i].texCoord = { 0.0f, 0.0f };
	}

	if (SetPDTStream(vertices, 8))
	{
		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
		_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
		STATEMANAGER.GetTransform().SetWorld(ms_matStack.back());

		SetDefaultIndexBuffer(DEFAULT_IB_FILL_CUBE);
		STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 0, 4 * 3);
	}
}

void CScreen::RenderLine2d(float sx, float sy, float ex, float ey, float z)
{
	RenderLine3d(sx, sy, z, ex, ey, z);
}

void CScreen::RenderBox2d(float sx, float sy, float ex, float ey, float z)
{
	RenderBox3d(sx, sy, z, ex, ey, z);
}

void CScreen::RenderBar2d(float sx, float sy, float ex, float ey, float z)
{
	RenderBar3d(sx, sy, z, ex, ey, z);
}

void CScreen::RenderGradationBar2d(float sx, float sy, float ex, float ey, DWORD dwStartColor, DWORD dwEndColor, float ez)
{
	RenderGradationBar3d(sx, sy, ez, ex, ey, ez, dwStartColor, dwEndColor);
}

void CScreen::RenderCircle2d(float fx, float fy, float fz, float fRadius, int iStep)
{
	int count;
	float theta, delta;
	float x, y, z;

	std::vector<XMFLOAT3> pts;

	pts.clear();
	pts.resize(iStep);

	theta = 0.0f;
	delta = 2.0f * XM_PI / float(iStep);

	for (count = 0; count < iStep; count++)
	{
		x = fx + fRadius * cosf(theta);
		y = fy + fRadius * sinf(theta);
		z = fz;

		pts[count] = XMFLOAT3(x, y, z);

		theta += delta;
	}

	for (count = 0; count < iStep - 1; count++)
	{
		RenderLine3d(
			pts[count].x, pts[count].y, pts[count].z,
			pts[count + 1].x, pts[count + 1].y, pts[count + 1].z
		);
	}

	RenderLine3d(
		pts[iStep - 1].x, pts[iStep - 1].y, pts[iStep - 1].z,
		pts[0].x, pts[0].y, pts[0].z
	);
}

void CScreen::RenderCircle3d(float fx, float fy, float fz, float fRadius, int iStep)
{
	int count;
	float theta, delta;
	std::vector<XMFLOAT3> pts;

	pts.clear();
	pts.resize(iStep);

	theta = 0.0f;
	delta = 2.0f * XM_PI / float(iStep);

	const XMFLOAT4X4& c_rmatInvView = CCameraManager::Instance().GetCurrentCamera()->GetBillboardMatrix();
	const XMMATRIX matInvView = XMLoadFloat4x4(&c_rmatInvView);

	for (count = 0; count < iStep; count++)
	{
		pts[count] = XMFLOAT3(fRadius * cosf(theta), fRadius * sinf(theta), 0.0f);
		XMStoreFloat3(&pts[count], XMVector3TransformCoord(XMLoadFloat3(&pts[count]), matInvView));

		theta += delta;
	}

	for (count = 0; count < iStep - 1; count++)
		RenderLine3d(fx + pts[count].x, fy + pts[count].y, fz + pts[count].z, fx + pts[count + 1].x, fy + pts[count + 1].y, fz + pts[count + 1].z);

	RenderLine3d(fx + pts[iStep - 1].x, fy + pts[iStep - 1].y, fz + pts[iStep - 1].z, fx + pts[0].x, fy + pts[0].y, fz + pts[0].z);
}

class CD3DXMeshRenderingOption : public CScreen
{
public:
	CD3DXMeshRenderingOption(D3D11_FILL_MODE eFillMode, const XMFLOAT4X4& c_rmatWorld)
	{
		STATEMANAGER.GetRaster().Push();
		STATEMANAGER.GetTransform().Push();

		STATEMANAGER.GetRaster().SetFillMode(eFillMode);
		STATEMANAGER.GetTransform().SetWorld(c_rmatWorld);

		STATEMANAGER.SetTexture(0, NULL);
		STATEMANAGER.SetTexture(1, NULL);
	}

	virtual ~CD3DXMeshRenderingOption()
	{
		STATEMANAGER.GetTransform().Restore();
		STATEMANAGER.GetRaster().Restore();
	}
};

void CScreen::RenderTextureBox(float sx, float sy, float ex, float ey, float z, float su, float sv, float eu, float ev)
{
	assert(ms_lpd3d11Device != NULL);

	TPDTVertex vertices[4];

	vertices[0].position = TPosition(sx, sy, z);
	vertices[0].diffuse = ms_diffuseColor;
	vertices[0].texCoord = TTextureCoordinate(su, sv);
	
	vertices[1].position = TPosition(ex, sy, z);
	vertices[1].diffuse = ms_diffuseColor;
	vertices[1].texCoord = TTextureCoordinate(eu, sv);
	
	vertices[2].position = TPosition(sx, ey, z);
	vertices[2].diffuse = ms_diffuseColor;
	vertices[2].texCoord = TTextureCoordinate(su, ev);
	
	vertices[3].position = TPosition(ex, ey, z);
	vertices[3].diffuse = ms_diffuseColor;
	vertices[3].texCoord = TTextureCoordinate(eu, ev);

			_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);

	SetDefaultIndexBuffer(DEFAULT_IB_FILL_RECT);
	if (SetPDTStream(vertices, 4))
		STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 0, 2);
}


void CScreen::RenderBillboard(XMFLOAT3 * Position, XMFLOAT4 & Color)
{
	assert(ms_lpd3d11Device != NULL);
	
	TPDTVertex vertices[4];
	vertices[0].position = TPosition(Position[0].x, Position[0].y, Position[0].z);
	vertices[0].diffuse = ColorToUint(Color);
	vertices[0].texCoord = TTextureCoordinate(0, 0);
	
	vertices[1].position = TPosition(Position[1].x, Position[1].y, Position[1].z);
	vertices[1].diffuse = ColorToUint(Color);
	vertices[1].texCoord = TTextureCoordinate(1, 0);
	
	vertices[2].position = TPosition(Position[2].x, Position[2].y, Position[2].z);
	vertices[2].diffuse = ColorToUint(Color);
	vertices[2].texCoord = TTextureCoordinate(0, 1);
	
	vertices[3].position = TPosition(Position[3].x, Position[3].y, Position[3].z);
	vertices[3].diffuse = ColorToUint(Color);
	vertices[3].texCoord = TTextureCoordinate(1, 1);
	
			_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);

	SetDefaultIndexBuffer(DEFAULT_IB_FILL_RECT);
	if (SetPDTStream(vertices, 4))
		STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 0, 2);
}

void CScreen::DrawMinorGrid(float xMin, float yMin, float xMax, float yMax, float xminorStep, float yminorStep, float zPos)
{
	float x, y;
	
	for (y = yMin; y <= yMax; y += yminorStep)
		RenderLine2d(xMin, y, xMax, y, zPos);

	for (x = xMin; x <= xMax; x += xminorStep)
		RenderLine2d(x, yMin, x, yMax, zPos);
}

void CScreen::DrawGrid(float xMin, float yMin, float xMax, float yMax, float xmajorStep, float ymajorStep, float xminorStep, float yminorStep, float zPos)
{
	xMin*=xminorStep;
	xMax*=xminorStep;
	yMin*=yminorStep;
	yMax*=yminorStep;
	xmajorStep*=xminorStep;
	ymajorStep*=yminorStep;
	
	float x, y;
	
	SetDiffuseColor(0.5f, 0.5f, 0.5f);
	DrawMinorGrid(xMin, yMin, xMax, yMax, xminorStep, yminorStep, zPos);
	
	SetDiffuseColor(0.7f, 0.7f, 0.7f);
	for (y = 0.0f; y >= yMin; y -= ymajorStep)
		RenderLine2d(xMin, y, xMax, y, zPos);
	
	for (y = 0.0f; y <= yMax; y += ymajorStep)
		RenderLine2d(xMin, y, xMax, y, zPos);
	
	for (x = 0.0f; x >= xMin; x -= xmajorStep)
		RenderLine2d(x, yMin, x, yMax, zPos);
	
	for (x = 0.0f; x <= yMax; x += xmajorStep)
		RenderLine2d(x, yMin, x, yMax, zPos);

	SetDiffuseColor(1.0f, 1.0f, 1.0f);
	RenderLine2d(xMin, 0.0f, xMax, 0.0f, zPos);
	RenderLine2d(0.0f, yMin, 0.0f, yMax, zPos);
}

void CScreen::SetCursorPosition(int x, int y, int hres, int vres)
{
	XMFLOAT3 v;
	v.x = -(((2.0f * x) / hres) - 1.0f) / ms_matProj._11;
	v.y = (((2.0f * y) / vres) - 1.0f) / ms_matProj._22;
	v.z = 1.0f;

	XMFLOAT4X4 matViewInverse = ms_matInverseView;

	ms_vtPickRayDir.x = v.x * matViewInverse._11 + v.y * matViewInverse._21 + v.z * matViewInverse._31;
	ms_vtPickRayDir.y = v.x * matViewInverse._12 + v.y * matViewInverse._22 + v.z * matViewInverse._32;
	ms_vtPickRayDir.z = v.x * matViewInverse._13 + v.y * matViewInverse._23 + v.z * matViewInverse._33;

	ms_vtPickRayOrig.x = matViewInverse._41;
	ms_vtPickRayOrig.y = matViewInverse._42;
	ms_vtPickRayOrig.z = matViewInverse._43;

	ms_Ray.SetStartPoint(ms_vtPickRayOrig);

	XMFLOAT3 vNegPickRayDir;
	XMStoreFloat3(&vNegPickRayDir, -XMLoadFloat3(&ms_vtPickRayDir));

	ms_Ray.SetDirection(vNegPickRayDir, 51200.0f);
}

bool CScreen::GetCursorPosition(float* px, float* py, float* pz)
{
	if (!GetCursorXYPosition(px, py)) return false;
	if (!GetCursorZPosition(pz)) return false;

	return true;
}

bool CScreen::GetCursorXYPosition(float* px, float* py)
{
	XMFLOAT3 v3Eye = CCameraManager::Instance().GetCurrentCamera()->GetEye();

	TPosition posVertices[4];
	posVertices[0] = TPosition(v3Eye.x-90000000.0f, v3Eye.y+90000000.0f, 0.0f);
	posVertices[1] = TPosition(v3Eye.x-90000000.0f, v3Eye.y-90000000.0f, 0.0f);
	posVertices[2] = TPosition(v3Eye.x+90000000.0f, v3Eye.y+90000000.0f, 0.0f);
	posVertices[3] = TPosition(v3Eye.x+90000000.0f, v3Eye.y-90000000.0f, 0.0f);

	static const WORD sc_awFillRectIndices[6] = { 0, 2, 1, 2, 3, 1, };

	float u, v, t;	
	for (int i = 0; i < 2; ++i)
	{
		if (IntersectTriangle(ms_vtPickRayOrig, ms_vtPickRayDir,
							 posVertices[sc_awFillRectIndices[i*3+0]],
							 posVertices[sc_awFillRectIndices[i*3+1]],
							 posVertices[sc_awFillRectIndices[i*3+2]],
							 &u, &v, &t))
		{
			*px = ms_vtPickRayOrig.x + ms_vtPickRayDir.x * t;
			*py = ms_vtPickRayOrig.y + ms_vtPickRayDir.y * t;
			return true;
		}
	}
	return false;
}

bool CScreen::GetCursorZPosition(float* pz)
{
	XMFLOAT3 v3Eye = CCameraManager::Instance().GetCurrentCamera()->GetEye();

	TPosition posVertices[4];
	posVertices[0] = TPosition(v3Eye.x-90000000.0f, 0.0f, v3Eye.z+90000000.0f);
	posVertices[1] = TPosition(v3Eye.x-90000000.0f, 0.0f, v3Eye.z-90000000.0f);
	posVertices[2] = TPosition(v3Eye.x+90000000.0f, 0.0f, v3Eye.z+90000000.0f);
	posVertices[3] = TPosition(v3Eye.x+90000000.0f, 0.0f, v3Eye.z-90000000.0f);

	static const WORD sc_awFillRectIndices[6] = { 0, 2, 1, 2, 3, 1, };

	float u, v, t;
	for (int i = 0; i < 2; ++i)
	{
		if (IntersectTriangle(ms_vtPickRayOrig, ms_vtPickRayDir,
							 posVertices[sc_awFillRectIndices[i*3+0]],
							 posVertices[sc_awFillRectIndices[i*3+1]],
							 posVertices[sc_awFillRectIndices[i*3+2]],
							 &u, &v, &t))
		{
			*pz = ms_vtPickRayOrig.z + ms_vtPickRayDir.z * t;
			return true;
		}
	}
	return false;
}

void CScreen::GetPickingPosition(float t, float* x, float* y, float* z)
{
	*x = ms_vtPickRayOrig.x + ms_vtPickRayDir.x * t;
	*y = ms_vtPickRayOrig.y + ms_vtPickRayDir.y * t;
	*z = ms_vtPickRayOrig.z + ms_vtPickRayDir.z * t;
}

void CScreen::SetDiffuseColor(DWORD diffuseColor)
{
	ms_diffuseColor = diffuseColor;
}

void CScreen::SetDiffuseColor(float r, float g, float b, float a)
{
	ms_diffuseColor = GetColor(r, g, b, a);
}

void CScreen::SetClearColor(float r, float g, float b, float a)
{
	ms_clearColor = GetColor(r, g, b, a);
}

void CScreen::SetClearDepth(float depth)
{
	ms_clearDepth = depth;
}

void CScreen::SetClearStencil(DWORD stencil)
{
	ms_clearStencil = stencil;
}

void CScreen::ClearDepthBuffer()
{
	if (ms_lpd3d11Context && ms_lpd3d11DSV)
		ms_lpd3d11Context->ClearDepthStencilView(ms_lpd3d11DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, ms_clearDepth, (UINT8)ms_clearStencil);
}

void CScreen::Clear()
{
	if (ms_lpd3d11Context && ms_lpd3d11RTV)
	{
		float c[4] = {
			((ms_clearColor >> 16) & 0xFF) / 255.0f,
			((ms_clearColor >>  8) & 0xFF) / 255.0f,
			((ms_clearColor      ) & 0xFF) / 255.0f,
			((ms_clearColor >> 24) & 0xFF) / 255.0f,
		};
		ms_lpd3d11Context->ClearRenderTargetView(ms_lpd3d11RTV, c);
		if (ms_lpd3d11DSV)
			ms_lpd3d11Context->ClearDepthStencilView(ms_lpd3d11DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, ms_clearDepth, (UINT8)ms_clearStencil);
	}
}

BOOL CScreen::IsLostDevice()
{
	// D3D11 doesn't have lost devices in the D3D9 sense
	return FALSE;
}

BOOL CScreen::RestoreDevice()
{
	// D3D11 doesn't need device restore
	return TRUE;
}

bool CScreen::Begin()
{
	ResetFaceCount();

	if (!STATEMANAGER.BeginScene())
	{
		Tracenf("BeginScene FAILED\n");
		return false;
	}

	return true;
}

void CScreen::End()
{
	STATEMANAGER.EndScene();
	CGraphicDevice::FlushD3D11DebugMessages();
}

extern bool g_isBrowserMode;
extern RECT g_rcBrowser;

void CScreen::Show(HWND hWnd)
{
	if (!ms_lpd3d11SwapChain)
		return;

	ms_lpd3d11SwapChain->Present(0, 0);
}

void CScreen::Show(RECT * pSrcRect)
{
	if (ms_lpd3d11SwapChain)
		ms_lpd3d11SwapChain->Present(0, 0);
}

void CScreen::Show(RECT * pSrcRect, HWND hWnd)
{
	if (ms_lpd3d11SwapChain)
		ms_lpd3d11SwapChain->Present(0, 0);
}

void CScreen::ProjectPosition(float x, float y, float z, float* pfX, float* pfY)
{
	XMFLOAT3 Input(x, y, z);
	XMFLOAT3 Output;

	XMStoreFloat3(&Output, XMVector3Project(XMLoadFloat3(&Input), ms_Viewport.TopLeftX, ms_Viewport.TopLeftY, ms_Viewport.Width, ms_Viewport.Height, ms_Viewport.MinDepth, ms_Viewport.MaxDepth, XMLoadFloat4x4(&ms_matProj), XMLoadFloat4x4(&ms_matView), XMLoadFloat4x4(&ms_matWorld)));

	*pfX = Output.x;
	*pfY = Output.y;
}

void CScreen::ProjectPosition(float x, float y, float z, float* pfX, float* pfY, float* pfZ)
{
	XMFLOAT3 Input(x, y, z);
	XMFLOAT3 Output;

	XMStoreFloat3(&Output, XMVector3Project(XMLoadFloat3(&Input), ms_Viewport.TopLeftX, ms_Viewport.TopLeftY, ms_Viewport.Width, ms_Viewport.Height, ms_Viewport.MinDepth, ms_Viewport.MaxDepth, XMLoadFloat4x4(&ms_matProj), XMLoadFloat4x4(&ms_matView), XMLoadFloat4x4(&ms_matWorld)));

	*pfX = Output.x;
	*pfY = Output.y;
	*pfZ = Output.z;
}

void CScreen::UnprojectPosition(float x, float y, float z, float* pfX, float* pfY, float* pfZ)
{
	XMFLOAT3 Input(x, y, z);
	XMFLOAT3 Output;

	XMStoreFloat3(&Output, XMVector3Unproject(XMLoadFloat3(&Input), ms_Viewport.TopLeftX, ms_Viewport.TopLeftY, ms_Viewport.Width, ms_Viewport.Height, ms_Viewport.MinDepth, ms_Viewport.MaxDepth, XMLoadFloat4x4(&ms_matProj), XMLoadFloat4x4(&ms_matView), XMLoadFloat4x4(&ms_matWorld)));

	*pfX = Output.x;
	*pfY = Output.y;
	*pfZ = Output.z;
}

void CScreen::SetColorOperation()
{
	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);
	_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
}

void CScreen::SetDiffuseOperation()
{
	STATEMANAGER.SetTexture(0, NULL);
	_mgr->SetShader(VF_PDT, BLEND_MODULATE);
}

void CScreen::SetBlendOperation()
{
	STATEMANAGER.SetTexture(0, NULL);
	_mgr->SetShader(VF_PDT, BLEND_MODULATE);
}

void CScreen::SetOneColorOperation(XMFLOAT4& rColor)
{
	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);
	ms_diffuseColor = ColorToUint(rColor);
	_mgr->SetShader(VF_PDT, BLEND_UI_DIFFUSE);
}

void CScreen::SetAddColorOperation(XMFLOAT4& rColor)
{
	ms_diffuseColor = ColorToUint(rColor);
	_mgr->SetShader(VF_PDT, BLEND_ADD);
}

void CScreen::Identity()
{
	STATEMANAGER.GetTransform().SetWorld(ms_matIdentity);
}

CScreen::CScreen()
{
}

CScreen::~CScreen()
{
}
//void BuildViewFrustum() { ms_frustum.BuildViewFrustum(ms_matView*ms_matProj); }

void CScreen::BuildViewFrustum()
{
	const XMFLOAT3& c_rv3Eye = CCameraManager::Instance().GetCurrentCamera()->GetEye();
	const XMFLOAT3& c_rv3View = CCameraManager::Instance().GetCurrentCamera()->GetView();

	XMFLOAT4X4 vv;
	XMStoreFloat4x4(&vv, XMMatrixMultiply(XMLoadFloat4x4(&ms_matView), XMLoadFloat4x4(&ms_matProj)));

	ms_frustum.BuildViewFrustum2(vv, ms_fNearY, ms_fFarY, ms_fFieldOfView, ms_fAspect, c_rv3Eye, c_rv3View);
}