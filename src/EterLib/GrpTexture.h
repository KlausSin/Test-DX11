#pragma once

#include "GrpBase.h"

class CGraphicTexture : public CGraphicBase
{
public:
    CGraphicTexture();
    virtual ~CGraphicTexture();

    bool IsEmpty() const;

    int GetWidth() const;
    int GetHeight() const;

    ID3D11ShaderResourceView* GetSRV() const;
    void SetTextureStage(int stage) const;
    void DestroyDeviceObjects();

protected:
    void Initialize();
    void Destroy();

protected:
    bool m_bEmpty;

    int m_width;
    int m_height;

    ID3D11ShaderResourceView* m_lpSRV;
};
