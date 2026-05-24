///////////////////////////////////////////////////////////////////////  
//	CSpeedTreeMaterial Class
//
//	(c) 2003 IDV, Inc.
//
//	This class is provided to illustrate one way to incorporate
//	SpeedTreeRT into an OpenGL application.  All of the SpeedTreeRT
//	calls that must be made on a per tree basis are done by this class.
//	Calls that apply to all trees (i.e. static SpeedTreeRT functions)
//	are made in the functions in main.cpp.
//
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com

#pragma once


///////////////////////////////////////////////////////////////////////  
//	Include Files

#include "EterLib/D3DXMathCompat.h"
#include "qMin32Lib/Core.h"
#include <qMin32Lib/ConstantBufferManager.h>

///////////////////////////////////////////////////////////////////////  
//	class CSpeedTreeMaterial declaration/definiton

class CSpeedTreeMaterial
{
public:
	CSpeedTreeMaterial()
	{
		m_material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		m_material.ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		m_material.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 5.0f);
		m_material.emissive = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		m_material.params = XMFLOAT4(5.0f, 0.0f, 0.0f, 0.0f);
	}

	void Set(const float* pMaterialArray)
	{
		memcpy(&m_material.diffuse, pMaterialArray, 3 * sizeof(float));
		m_material.diffuse.w = 1.0f;

		memcpy(&m_material.ambient, pMaterialArray + 3, 3 * sizeof(float));
		m_material.ambient.w = 1.0f;

		memcpy(&m_material.specular, pMaterialArray + 6, 3 * sizeof(float));
		m_material.specular.w = pMaterialArray[12];

		memcpy(&m_material.emissive, pMaterialArray + 9, 3 * sizeof(float));
		m_material.emissive.w = 1.0f;

		m_material.params.x = pMaterialArray[12];
	}

	const CBMaterialX& Get() const { return m_material; }
	CBMaterialX& Get() { return m_material; }

private:
	CBMaterialX m_material;
};
