#include "StdAfx.h"
#include "EterBase/Stl.h"
#include "EterBase/FileLoader.h"
#include "GrpSubImage.h"
#include "ResourceManager.h"

char CGraphicSubImage::m_SearchPath[256] = "D:/Ymir Work/UI/";

CGraphicSubImage::CGraphicSubImage(const char* fileName)
	: CGraphicImage(fileName)
{
}

CGraphicSubImage::~CGraphicSubImage()
{
	OnClear();
}

CGraphicSubImage::TType CGraphicSubImage::Type()
{
	static TType type = StringToType("CGraphicSubImage");
	return type;
}

bool CGraphicSubImage::CreateDeviceObjects()
{
	if (m_roImage.IsNull())
		return false;

	m_imageTexture.CreateFromTexturePointer(m_roImage->GetTexturePointer());
	return true;
}

void CGraphicSubImage::SetImagePointer(CGraphicImage* image)
{
	m_roImage.SetPointer(image);
	CreateDeviceObjects();
}

bool CGraphicSubImage::SetImageFileName(const char* fileName)
{
	CResource* resource = CResourceManager::Instance().GetResourcePointer(fileName);

	if (!resource || !resource->IsType(CGraphicImage::Type()))
		return false;

	SetImagePointer(static_cast<CGraphicImage*>(resource));
	return true;
}

void CGraphicSubImage::SetRectPosition(int left, int top, int right, int bottom)
{
	m_rect.left = left;
	m_rect.top = top;
	m_rect.right = right;
	m_rect.bottom = bottom;
}

void CGraphicSubImage::SetRectReference(const RECT& rect)
{
	m_rect = rect;
}

void CGraphicSubImage::SetSearchPath(const char* fileName)
{
	strncpy(m_SearchPath, fileName, sizeof(m_SearchPath) - 1);
	m_SearchPath[sizeof(m_SearchPath) - 1] = '\0';
}

bool CGraphicSubImage::OnLoad(int size, const void* data)
{
	if (!data || size <= 0)
		return false;

	CTokenVector tokenVector;
	std::map<std::string, std::string> tokenMap;

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(size, data);

	for (DWORD i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLine(i, &tokenVector))
			continue;

		if (tokenVector.size() != 2)
			continue;

		stl_lowers(tokenVector[0]);
		stl_lowers(tokenVector[1]);

		tokenMap[tokenVector[0]] = tokenVector[1];
	}

	const std::string title = tokenMap["title"];
	const std::string version = tokenMap["version"];
	const std::string image = tokenMap["image"];

	if (title != "subimage" || image.empty())
		return false;

	char imageFileName[MAX_PATH] = {};

	if (version == "2.0")
	{
		const std::string& subFileName = GetFileNameString();
		const size_t pos = subFileName.find_last_of("\\/");

		if (pos != std::string::npos)
		{
			const std::string path = subFileName.substr(0, pos + 1);
			_snprintf(imageFileName, sizeof(imageFileName), "%s%s", path.c_str(), image.c_str());
		}
		else
		{
			_snprintf(imageFileName, sizeof(imageFileName), "%s", image.c_str());
		}
	}
	else
	{
		_snprintf(imageFileName, sizeof(imageFileName), "%s%s", m_SearchPath, image.c_str());
	}

	if (!SetImageFileName(imageFileName))
		return false;

	SetRectPosition(
		atoi(tokenMap["left"].c_str()),
		atoi(tokenMap["top"].c_str()),
		atoi(tokenMap["right"].c_str()),
		atoi(tokenMap["bottom"].c_str()));

	return true;
}

void CGraphicSubImage::OnClear()
{
	m_roImage.SetPointer(nullptr);
	m_imageTexture.Destroy();
	ZeroMemory(&m_rect, sizeof(m_rect));
}

bool CGraphicSubImage::OnIsEmpty() const
{
	return m_roImage.IsNull() || m_roImage->IsEmpty();
}

bool CGraphicSubImage::OnIsType(TType type)
{
	if (type == CGraphicSubImage::Type())
		return true;

	return CGraphicImage::OnIsType(type);
}
