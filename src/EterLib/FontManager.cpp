#include "StdAfx.h"
#include "FontManager.h"

#include <algorithm>

namespace
{
	std::string ToLower(std::string text)
	{
		std::transform(
			text.begin(),
			text.end(),
			text.begin(),
			[](unsigned char c) { return static_cast<char>(tolower(c)); });

		return text;
	}

	std::wstring ToWide(const char* text)
	{
		if (!text || !text[0])
			return L"Tahoma";

		wchar_t buffer[LF_FACESIZE] = {};

		MultiByteToWideChar(
			CP_ACP,
			0,
			text,
			-1,
			buffer,
			LF_FACESIZE);

		return buffer;
	}
}

CFontManager::CFontManager()
	: m_initialized(false)
{
}

CFontManager::~CFontManager()
{
	Destroy();
}

CFontManager& CFontManager::Instance()
{
	static CFontManager instance;
	return instance;
}

bool CFontManager::Initialize()
{
	if (m_initialized)
		return true;

	RegisterDefaultFonts();

	m_initialized = true;
	return true;
}

void CFontManager::Destroy()
{
	m_fontNameMap.clear();
	m_initialized = false;
}

void CFontManager::RegisterDefaultFonts()
{
	m_fontNameMap["gulim"] = L"Gulim";
	m_fontNameMap["굴림"] = L"Gulim";
	m_fontNameMap["굴림체"] = L"GulimChe";

	m_fontNameMap["arial"] = L"Arial";
	m_fontNameMap["arial bold"] = L"Arial";
	m_fontNameMap["tahoma"] = L"Tahoma";
	m_fontNameMap["tahoma bold"] = L"Tahoma";
	m_fontNameMap["verdana"] = L"Verdana";
	m_fontNameMap["verdana bold"] = L"Verdana";
	m_fontNameMap["times new roman"] = L"Times New Roman";
	m_fontNameMap["courier new"] = L"Courier New";
	m_fontNameMap["segoe ui"] = L"Segoe UI";
}

std::wstring CFontManager::ResolveFontName(const char* faceName) const
{
	if (!faceName || !faceName[0])
		return L"Tahoma";

	const std::string key = ToLower(faceName);

	const auto it = m_fontNameMap.find(key);

	if (it != m_fontNameMap.end())
		return it->second;

	return ToWide(faceName);
}

HFONT CFontManager::CreateFontHandle(
	const char* faceName,
	int fontSize,
	bool bold,
	bool italic) const
{
	const std::wstring fontName = ResolveFontName(faceName);
	const int size = fontSize > 0 ? fontSize : 12;

	return CreateFontW(
		-size,
		0,
		0,
		0,
		bold ? FW_BOLD : FW_NORMAL,
		italic ? TRUE : FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		fontName.c_str());
}
