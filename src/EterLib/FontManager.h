#pragma once

#include <string>
#include <unordered_map>

class CFontManager
{
public:
	static CFontManager& Instance();

	bool Initialize();
	void Destroy();

	std::wstring ResolveFontName(const char* faceName) const;
	HFONT CreateFontHandle(const char* faceName, int fontSize, bool bold, bool italic) const;

private:
	CFontManager();
	~CFontManager();

	CFontManager(const CFontManager&) = delete;
	CFontManager& operator=(const CFontManager&) = delete;

	void RegisterDefaultFonts();

private:
	bool m_initialized;
	std::unordered_map<std::string, std::wstring> m_fontNameMap;
};
