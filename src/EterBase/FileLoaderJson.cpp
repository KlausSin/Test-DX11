#include "StdAfx.h"
#include "FileLoaderJson.h"
#include "Debug.h"

CMemoryJsonFileLoader::CMemoryJsonFileLoader()
{
    m_currentNode = simdjson::dom::element();
}

CMemoryJsonFileLoader::~CMemoryJsonFileLoader()
{
}

bool CMemoryJsonFileLoader::Bind(const void* pData, size_t size)
{
    if (!pData || !size)
        return false;

    m_buffer.assign(
        reinterpret_cast<const char*>(pData),
        size
    );

    try
    {
        m_padded = simdjson::padded_string(m_buffer);

        auto error = m_parser.parse(m_padded).get(m_document);
        m_currentNode = m_document;
        m_nodeStack.clear();
        if (error)
        {
            TraceError(
                "CMemoryJsonFileLoader::Bind - Parse error: %s",
                simdjson::error_message(error)
            );

            return false;
        }
    }
    catch (...)
    {
        TraceError("CMemoryJsonFileLoader::Bind - Exception");
        return false;
    }

    return true;
}

bool CMemoryJsonFileLoader::Load(const char* c_szFileName)
{
    TPackFile file;

    if (!CPackManager::Instance().GetFile(c_szFileName, file))
    {
        TraceError(
            "CMemoryJsonFileLoader::Load - Cannot load: %s",
            c_szFileName
        );

        return false;
    }

    return Bind(file.data(), file.size());
}

bool CMemoryJsonFileLoader::HasKey(const char* c_szKey)
{
    return !m_currentNode[c_szKey].error();
}

bool CMemoryJsonFileLoader::GetTokenBoolean(const char* c_szKey, BOOL* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    *pValue = BOOL(bool(v));

    return true;
}

bool CMemoryJsonFileLoader::GetTokenInteger(const char* c_szKey, int* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    *pValue = int(int64_t(v));

    return true;
}

bool CMemoryJsonFileLoader::GetTokenFloat(const char* c_szKey, float* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    *pValue = float(double(v));

    return true;
}

bool CMemoryJsonFileLoader::GetTokenDouble(const char* c_szKey, double* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    *pValue = double(v);

    return true;
}

bool CMemoryJsonFileLoader::GetTokenByte(const char* c_szKey, BYTE* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    *pValue = BYTE(int64_t(v));

    return true;
}

bool CMemoryJsonFileLoader::GetTokenDWORD(const char* c_szKey, DWORD* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    *pValue = DWORD(uint64_t(v));

    return true;
}

bool CMemoryJsonFileLoader::GetTokenString(const char* c_szKey, std::string* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    *pValue = std::string(v);

    return true;
}

bool CMemoryJsonFileLoader::ReadFloatArray(
    simdjson::dom::array arr,
    std::vector<float>& vec)
{
    vec.clear();

    for (auto x : arr)
        vec.push_back(float(double(x)));

    return !vec.empty();
}

bool CMemoryJsonFileLoader::GetTokenVector2(
    const char* c_szKey,
    XMFLOAT2* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    std::vector<float> vec;

    if (!ReadFloatArray(v.get_array(), vec))
        return false;

    if (vec.size() < 2)
        return false;

    pValue->x = vec[0];
    pValue->y = vec[1];

    return true;
}

bool CMemoryJsonFileLoader::GetTokenVector3(
    const char* c_szKey,
    XMFLOAT3* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    std::vector<float> vec;

    if (!ReadFloatArray(v.get_array(), vec))
        return false;

    if (vec.size() < 3)
        return false;

    pValue->x = vec[0];
    pValue->y = vec[1];
    pValue->z = vec[2];

    return true;
}

bool CMemoryJsonFileLoader::GetTokenVector4(
    const char* c_szKey,
    XMFLOAT4* pValue)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    std::vector<float> vec;

    if (!ReadFloatArray(v.get_array(), vec))
        return false;

    if (vec.size() < 4)
        return false;

    pValue->x = vec[0];
    pValue->y = vec[1];
    pValue->z = vec[2];
    pValue->w = vec[3];

    return true;
}

bool CMemoryJsonFileLoader::GetTokenColor(
    const char* c_szKey,
    XMFLOAT4* pValue)
{
    return GetTokenVector4(c_szKey, pValue);
}

bool CMemoryJsonFileLoader::GetTokenArray(
    const char* c_szKey,
    std::vector<float>* pVec)
{
    auto v = m_currentNode[c_szKey];

    if (v.error())
        return false;

    return ReadFloatArray(v.get_array(), *pVec);
}

simdjson::dom::element CMemoryJsonFileLoader::GetRoot()
{
    return m_document;
}

simdjson::dom::element CMemoryJsonFileLoader::GetElement(const char* c_szKey)
{
    simdjson::dom::element elem;
    m_currentNode[c_szKey].get(elem);
    return elem;
}

simdjson::dom::array CMemoryJsonFileLoader::GetArray(const char* c_szKey)
{
    simdjson::dom::array arr;
    m_currentNode[c_szKey].get(arr);
    return arr;
}

bool CMemoryJsonFileLoader::SetChildNode(const char* c_szKey)
{
    simdjson::dom::element child;

    if (m_currentNode[c_szKey].get(child))
        return false;

    m_nodeStack.push_back(m_currentNode);
    m_currentNode = child;

    return true;
}

void CMemoryJsonFileLoader::SetParentNode()
{
    if (m_nodeStack.empty())
        return;

    m_currentNode = m_nodeStack.back();
    m_nodeStack.pop_back();
}
