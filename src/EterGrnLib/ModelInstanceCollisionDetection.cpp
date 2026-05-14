#include "Stdafx.h"
#include "ModelInstance.h"
#include "Model.h"

void CGrannyModelInstance::MakeBoundBox(TBoundBox* pBoundBox, 
										 const float* mat, 
										 const float* OBBMin, 
										 const float* OBBMax, 
										 XMFLOAT3* vtMin, 
										 XMFLOAT3* vtMax)
{
	pBoundBox->sx = OBBMin[0] * mat[0] + OBBMin[1] * mat[4] + OBBMin[2] * mat[8] + mat[12];
	pBoundBox->sy = OBBMin[0] * mat[1] + OBBMin[1] * mat[5] + OBBMin[2] * mat[9] + mat[13];
	pBoundBox->sz = OBBMin[0] * mat[2] + OBBMin[1] * mat[6] + OBBMin[2] * mat[10] + mat[14];

	pBoundBox->ex = OBBMax[0] * mat[0] + OBBMax[1] * mat[4] + OBBMax[2] * mat[8] + mat[12];
	pBoundBox->ey = OBBMax[0] * mat[1] + OBBMax[1] * mat[5] + OBBMax[2] * mat[9] + mat[13];
	pBoundBox->ez = OBBMax[0] * mat[2] + OBBMax[1] * mat[6] + OBBMax[2] * mat[10] + mat[14];

	vtMin->x = std::min(vtMin->x, pBoundBox->sx);
	vtMin->x = std::min(vtMin->x, pBoundBox->ex);
	vtMin->y = std::min(vtMin->y, pBoundBox->sy);
	vtMin->y = std::min(vtMin->y, pBoundBox->ey);
	vtMin->z = std::min(vtMin->z, pBoundBox->sz);
	vtMin->z = std::min(vtMin->z, pBoundBox->ez);

	vtMax->x = std::max(vtMax->x, pBoundBox->sx);
	vtMax->x = std::max(vtMax->x, pBoundBox->ex);
	vtMax->y = std::max(vtMax->y, pBoundBox->sy);
	vtMax->y = std::max(vtMax->y, pBoundBox->ey);
	vtMax->z = std::max(vtMax->z, pBoundBox->sz);
	vtMax->z = std::max(vtMax->z, pBoundBox->ez);
}

bool CGrannyModelInstance::Intersect(const XMFLOAT4X4* c_pMatrix, float* /*pu*/, float* /*pv*/, float* pt)
{
	if (!m_pgrnModelInstance)
		return false;

	float u, v, t;
	*pt = 100000000.0f;

	const float max = 10000000.0f;

	XMFLOAT3 vtMin(max, max, max);
	XMFLOAT3 vtMax(-max, -max, -max);

	static stl_stack_pool<TBoundBox> s_boundBoxPool(1024);
	s_boundBoxPool.clear();

	int meshCount = m_pModel->GetMeshCount();

	for (int m = 0; m < meshCount; ++m)
	{
		const granny_mesh* pgrnMesh = m_pModel->GetGrannyModelPointer()->MeshBindings[m].Mesh;

		for (int b = 0; b < pgrnMesh->BoneBindingCount; ++b)
		{
			const granny_bone_binding& rgrnBoneBinding = pgrnMesh->BoneBindings[b];

			TBoundBox* pBoundBox = s_boundBoxPool.alloc();

			float* Transform = GrannyGetWorldPose4x4(__GetWorldPosePtr(), __GetMeshBoneIndices(m)[b]);

			MakeBoundBox(
				pBoundBox,
				Transform,
				rgrnBoneBinding.OBBMin,
				rgrnBoneBinding.OBBMax,
				&vtMin,
				&vtMax);

			pBoundBox->meshIndex = m;
			pBoundBox->boneIndex = b;
		}
	}

	if (!IntersectCube(
		c_pMatrix,
		vtMin.x, vtMin.y, vtMin.z,
		vtMax.x, vtMax.y, vtMax.z,
		ms_vtPickRayOrig,
		ms_vtPickRayDir,
		&u, &v, &t))
	{
		return false;
	}

	*pt = t;
	return true;
}

#include "EterBase/Timer.h"

void CGrannyModelInstance::GetBoundBox(XMFLOAT3* vtMin, XMFLOAT3* vtMax)
{
	if (!m_pgrnModelInstance)
		return;

	TBoundBox BoundBox;

	vtMin->x = vtMin->y = vtMin->z = +100000.0f;
	vtMax->x = vtMax->y = vtMax->z = -100000.0f;

	int meshCount = m_pModel->GetMeshCount();
	for (int m = 0; m < meshCount; ++m)
	{
		//const CGrannyMesh* pMesh = m_pModel->GetMeshPointer(m);
		const granny_mesh* pgrnMesh = m_pModel->GetGrannyModelPointer()->MeshBindings[m].Mesh;

		// WORK
		int* boneIndices = __GetMeshBoneIndices(m);
		// END_OF_WORK
		for (int b = 0; b < pgrnMesh->BoneBindingCount; ++b)
		{
			const granny_bone_binding& rgrnBoneBinding = pgrnMesh->BoneBindings[b];

			MakeBoundBox(&BoundBox,
						 GrannyGetWorldPose4x4(__GetWorldPosePtr(), boneIndices[b]),
						 rgrnBoneBinding.OBBMin, rgrnBoneBinding.OBBMax, vtMin, vtMax);
		}
	}
}

bool CGrannyModelInstance::GetMeshMatrixPointer(int iMesh, const XMFLOAT4X4** c_ppMatrix) const
{
	if (!m_pgrnModelInstance)
		return false;

	int meshCount = m_pModel->GetMeshCount();

	if (meshCount <= 0)
		return false;

	*c_ppMatrix = reinterpret_cast<const XMFLOAT4X4*>(
		GrannyGetWorldPose4x4(__GetWorldPosePtr(), __GetMeshBoneIndices(iMesh)[0])
		);

	return true;
}

