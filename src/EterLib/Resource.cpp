#include "StdAfx.h"

#include "Resource.h"
#include "ResourceManager.h"

#include "PackLib/PackManager.h"
#include "EterBase/CRC32.h"
#include "EterBase/Stl.h"

bool CResource::ms_bDeleteImmediately = false;

namespace
{
    DWORD NowMS()
    {
        using namespace std::chrono;
        return static_cast<DWORD>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }
}

CResource::CResource(const char* c_szFileName)
{
    SetFileName(c_szFileName);
}

void CResource::SetDeleteImmediately(bool isSet) noexcept
{
    ms_bDeleteImmediately = isSet;
}

void CResource::OnConstruct()
{
    CReferenceObject::OnConstruct();
    Load();
}

void CResource::OnSelfDestruct()
{
    if (ms_bDeleteImmediately)
        Clear();
    else
        CResourceManager::Instance().ReserveDeletingResource(this);
}

void CResource::Load()
{
    if (me_state != STATE_EMPTY)
        return;

    me_state = STATE_LOAD;
    const DWORD start = NowMS();
    TPackFile file;

    if (CPackManager::Instance().GetFile(GetFileName(), file))
    {
        m_dwLoadCostMiliiSecond = NowMS() - start;
        me_state = OnLoad(static_cast<int>(file.size()), file.data()) ? STATE_EXIST : STATE_ERROR;
        if (me_state == STATE_ERROR)
            Tracef("CResource::Load Error %s\n", GetFileName());
        return;
    }

    me_state = OnLoad(0, nullptr) ? STATE_EXIST : STATE_ERROR;
    if (me_state == STATE_ERROR)
        Tracef("CResource::Load file not exist %s\n", GetFileName());
}

void CResource::Reload()
{
    Clear();
    Tracef("CResource::Reload %s\n", GetFileName());
    Load();
}

CResource::TType CResource::StringToType(const char* c_szType)
{
    return c_szType ? GetCRC32(c_szType, std::strlen(c_szType)) : 0u;
}

CResource::TType CResource::Type()
{
    static const TType s_type = StringToType("CResource");
    return s_type;
}

int CResource::ConvertPathName(const char* c_szPathName, char* pszRetPathName, int retLen)
{
    if (!c_szPathName || !pszRetPathName || retLen <= 0)
        return 0;

    int len = 0;
    for (; c_szPathName[len] && len < retLen - 1; ++len)
    {
        const unsigned char ch = static_cast<unsigned char>(c_szPathName[len]);
        pszRetPathName[len] = ch == '/' ? '\\' : static_cast<char>(std::tolower(ch));
    }

    pszRetPathName[len] = '\0';
    return len;
}

void CResource::SetFileName(const char* c_szFileName)
{
    m_stFileName = c_szFileName ? c_szFileName : "";
}

void CResource::Clear()
{
    OnClear();
    me_state = STATE_EMPTY;
}

bool CResource::IsType(TType type)
{
    return OnIsType(type);
}

bool CResource::OnIsType(TType type)
{
    return Type() == type;
}

bool CResource::IsData() const noexcept
{
    return me_state != STATE_EMPTY && me_state != STATE_ERROR;
}

bool CResource::IsEmpty() const
{
    return OnIsEmpty();
}

bool CResource::CreateDeviceObjects()
{
    return true;
}

void CResource::DestroyDeviceObjects()
{
}
