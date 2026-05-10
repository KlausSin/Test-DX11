#include "StdAfx.h"
#include "Eterbase/Debug.h"
#include "Thing.h"
#include "ThingInstance.h"

CGraphicThing::CGraphicThing(const char* c_szFileName) : CResource(c_szFileName)
{
	Initialize();	
}

CGraphicThing::~CGraphicThing()
{
	//OnClear();
	Clear();
}

void CGraphicThing::Initialize()
{
	m_pgrnFile = nullptr;
	m_pgrnFileInfo = nullptr;
	m_pgrnAni = nullptr;

	m_models.reset();
	m_motions.reset();
}

void CGraphicThing::OnClear()
{
	m_motions.reset();
	m_models.reset();

	if (m_pgrnFile)
		GrannyFreeFile(m_pgrnFile);

	Initialize();
}

CGraphicThing::TType CGraphicThing::Type()
{
	static TType s_type = StringToType("CGraphicThing");
	return s_type;
}

bool CGraphicThing::OnIsEmpty() const
{
	return m_pgrnFile ? false : true;
}

bool CGraphicThing::OnIsType(TType type)
{
	if (CGraphicThing::Type() == type)
		return true;

	return CResource::OnIsType(type);
}

bool CGraphicThing::CreateDeviceObjects()
{
	if (!m_pgrnFileInfo)
		return true;
	
	for (int m = 0; m < m_pgrnFileInfo->ModelCount; ++m)
	{
		CGrannyModel& rModel = m_models[static_cast<size_t>(m)];
		rModel.CreateDeviceObjects();
	}

	return true;
}

void CGraphicThing::DestroyDeviceObjects()
{
	if (!m_pgrnFileInfo)
		return;

	for (int m = 0; m < m_pgrnFileInfo->ModelCount; ++m)
	{
		CGrannyModel& rModel = m_models[static_cast<size_t>(m)];
		rModel.DestroyDeviceObjects();
	}
}

bool CGraphicThing::CheckModelIndex(int iModel) const
{
	if (!m_pgrnFileInfo)
	{
		Tracef("m_pgrnFileInfo == nullptr: %s\n", GetFileName());
		return false;
	}

	assert(m_pgrnFileInfo != nullptr);

	if (iModel < 0)
		return false;

	if (iModel >= m_pgrnFileInfo->ModelCount)
		return false;

	return true;
}

bool CGraphicThing::CheckMotionIndex(int iMotion) const
{
	// Temporary
	if (!m_pgrnFileInfo)
		return false;
	// Temporary

	assert(m_pgrnFileInfo != nullptr);

	if (iMotion < 0)
		return false;
	
	if (iMotion >= m_pgrnFileInfo->AnimationCount)
		return false;

	return true;
}

CGrannyModel * CGraphicThing::GetModelPointer(int iModel)
{	
	assert(CheckModelIndex(iModel));
	assert(m_models != nullptr);
	return &m_models[static_cast<size_t>(iModel)];
}

CGrannyMotion * CGraphicThing::GetMotionPointer(int iMotion)
{
	assert(CheckMotionIndex(iMotion));

	if (iMotion >= m_pgrnFileInfo->AnimationCount)
		return nullptr;

	assert(m_motions != nullptr);
	return &m_motions[static_cast<size_t>(iMotion)];
}

int CGraphicThing::GetModelCount() const
{
	if (!m_pgrnFileInfo)
		return 0;

	return (m_pgrnFileInfo->ModelCount);
}

int CGraphicThing::GetMotionCount() const
{
	if (!m_pgrnFileInfo)
		return 0;

	return (m_pgrnFileInfo->AnimationCount);
}

bool CGraphicThing::OnLoad(int iSize, const void * c_pvBuf)
{
	if (!c_pvBuf)
		return false;

	m_pgrnFile = GrannyReadEntireFileFromMemory(iSize, (void *) c_pvBuf);

	if (!m_pgrnFile)
		return false;

    m_pgrnFileInfo = GrannyGetFileInfo(m_pgrnFile);

	if (!m_pgrnFileInfo)
		return false;

	LoadModels();
	LoadMotions();
	return true;
}

// SUPPORT_LOCAL_TEXTURE
static std::string gs_modelLocalPath;

const std::string& GetModelLocalPath()
{
	return gs_modelLocalPath;
}
// END_OF_SUPPORT_LOCAL_TEXTURE

bool CGraphicThing::LoadModels()
{
	if (!m_pgrnFile)
		return false;

	m_models.reset();

	if (m_pgrnFileInfo->ModelCount <= 0)
		return false;

	const std::string& fileName = GetFileNameString();

	if (fileName.length() > 2 && fileName[1] != ':')
	{
		const auto sepPos = fileName.rfind('\\');
		if (sepPos != std::string::npos)
			gs_modelLocalPath.assign(fileName, 0, sepPos + 1);
	}
	else
	{
		const auto sepPos = fileName.rfind('\\');
		if (sepPos != std::string::npos)
			gs_modelLocalPath.assign(fileName, 0, sepPos + 1);
	}

	const int modelCount = m_pgrnFileInfo->ModelCount;

	m_models = std::make_unique<CGrannyModel[]>(static_cast<size_t>(modelCount));

	for (int m = 0; m < modelCount; ++m)
	{
		CGrannyModel& model = m_models[static_cast<size_t>(m)];
		granny_model* grannyModel = m_pgrnFileInfo->Models[m];

		if (!model.CreateFromGrannyModelPointer(grannyModel))
			return false;
	}

	GrannyFreeFileSection(m_pgrnFile, GrannyStandardRigidVertexSection);
	GrannyFreeFileSection(m_pgrnFile, GrannyStandardRigidIndexSection);
	GrannyFreeFileSection(m_pgrnFile, GrannyStandardDeformableIndexSection);
	GrannyFreeFileSection(m_pgrnFile, GrannyStandardTextureSection);

	return true;
}

bool CGraphicThing::LoadMotions()
{
	assert(m_pgrnFile != nullptr);
	assert(m_motions == nullptr);

	if (m_pgrnFileInfo->AnimationCount <= 0)
		return false;
	
	int motionCount = m_pgrnFileInfo->AnimationCount;

	m_motions = std::make_unique<CGrannyMotion[]>(static_cast<size_t>(motionCount));
	
	for (int m = 0; m < motionCount; ++m)
		if (!m_motions[static_cast<size_t>(m)].BindGrannyAnimation(m_pgrnFileInfo->Animations[m]))
			return false;

	return true;
}
