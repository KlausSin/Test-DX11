#pragma once

#include <windows.h>
#include <vector>
#include <map>
#include <string>

#include "simdjson.h"

#include "Stl.h"
#include "PackLib/PackManager.h"
#include <DirectXMath.h>
using namespace DirectX;

typedef std::map<std::string, std::string> TStringMap;

class CMemoryJsonFileLoader
{
public:
    CMemoryJsonFileLoader();
    virtual ~CMemoryJsonFileLoader();

    bool Bind(const void* pData, size_t size);
    bool Load(const char* c_szFileName);

    bool HasKey(const char* c_szKey);

    bool GetTokenBoolean(const char* c_szKey, BOOL* pValue);
    bool GetTokenInteger(const char* c_szKey, int* pValue);
    bool GetTokenFloat(const char* c_szKey, float* pValue);
    bool GetTokenDouble(const char* c_szKey, double* pValue);
    bool GetTokenByte(const char* c_szKey, BYTE* pValue);
    bool GetTokenDWORD(const char* c_szKey, DWORD* pValue);

    bool GetTokenString(const char* c_szKey, std::string* pValue);

    bool GetTokenVector2(const char* c_szKey, XMFLOAT2* pValue);
    bool GetTokenVector3(const char* c_szKey, XMFLOAT3* pValue);
    bool GetTokenVector4(const char* c_szKey, XMFLOAT4* pValue);

    bool GetTokenColor(const char* c_szKey, XMFLOAT4* pValue);

    bool GetTokenArray(const char* c_szKey, std::vector<float>* pVec);

    simdjson::dom::element GetRoot();
    simdjson::dom::element GetElement(const char* c_szKey);
    simdjson::dom::array GetArray(const char* c_szKey);
    bool SetChildNode(const char* c_szKey);
    void SetParentNode();

private:
    bool ReadFloatArray(simdjson::dom::array arr, std::vector<float>& vec);

private:
    std::string                 m_buffer;
    simdjson::padded_string     m_padded;

    simdjson::dom::parser       m_parser;
    simdjson::dom::element      m_document;

    std::vector<simdjson::dom::element> m_nodeStack;
    simdjson::dom::element m_currentNode;
};
