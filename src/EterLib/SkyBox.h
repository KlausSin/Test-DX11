#pragma once

#include "GrpBase.h"
#include "GrpScreen.h"
#include "GrpImageInstance.h"
#include "ColorTransitionHelper.h"

#include <array>
#include <map>
#include <string>
#include <vector>

struct TColor
{
	TColor(float _r = 0.0f, float _g = 0.0f, float _b = 0.0f, float _a = 0.0f) : r(_r), g(_g), b(_b), a(_a) {}
	float r, g, b, a;
};

struct TGradientColor
{
	TColor m_FirstColor;
	TColor m_SecondColor;
};

using TVectorGradientColor = std::vector<TGradientColor>;
using TVectorGradientIterator = TVectorGradientColor::iterator;

class CSkyObjectQuad
{
public:
	CSkyObjectQuad();
	~CSkyObjectQuad() = default;

	void Clear(unsigned char vertexIndex, float r, float g, float b, float a);
	void SetSrcColor(unsigned char vertexIndex, float r, float g, float b, float a);
	void SetTransition(unsigned char vertexIndex, float r, float g, float b, float a, DWORD duration);
	void SetVertex(unsigned char vertexIndex, const TPDTVertex& vertex);
	void StartTransition();
	bool Update();
	void Render();

private:
	std::array<TPDTVertex, 4> m_Vertex{};
	std::array<TIndex, 4> m_Indices{};
	std::array<CColorTransitionHelper, 4> m_Helper{};
};

class CSkyObject : public CScreen
{
public:
	enum
	{
		SKY_RENDER_MODE_DEFAULT,
		SKY_RENDER_MODE_DIFFUSE,
		SKY_RENDER_MODE_TEXTURE,
		SKY_RENDER_MODE_MODULATE,
		SKY_RENDER_MODE_MODULATE2X,
		SKY_RENDER_MODE_MODULATE4X,
	};

	CSkyObject();
	~CSkyObject() = default;

	virtual void Destroy() = 0;
	virtual void Render() = 0;
	virtual void Render(const RenderContext& ctx) = 0;
	virtual void Update();
	virtual void Update(const RenderContext& ctx);
	virtual void StartTransition();

	void SetRenderMode(unsigned char renderMode) { m_ucRenderMode = renderMode; }
	bool isTransitionStarted() const { return m_bTransitionStarted; }

protected:
	struct TSkyObjectFace;
	using TSkyObjectQuadVector = std::vector<CSkyObjectQuad>;
	using TGraphicImageInstanceMap = std::map<std::string, CGraphicImageInstance*>;

	struct TSkyObjectFace
	{
		void StartTransition();
		bool Update();
		void Render();

		std::string m_strfacename;
		std::string m_strFaceTextureFileName;
		TSkyObjectQuadVector m_SkyObjectQuadVector;
	};

	CGraphicImageInstance* GenerateTexture(const char* filename);
	void DeleteTexture(CGraphicImageInstance* imageInstance);

protected:
	TSkyObjectFace m_FaceCloud;
	XMFLOAT4X4 m_matWorldCloud{};
	XMFLOAT4X4 m_matTextureCloud{};
	XMFLOAT3 m_v3PositionCloud{};

	float m_fCloudScaleX = 1.0f;
	float m_fCloudScaleY = 1.0f;
	float m_fCloudHeight = 0.0f;
	float m_fCloudTextureScaleX = 1.0f;
	float m_fCloudTextureScaleY = 1.0f;
	float m_fCloudScrollSpeedU = 0.0f;
	float m_fCloudScrollSpeedV = 0.0f;
	float m_fCloudPositionU = 0.0f;
	float m_fCloudPositionV = 0.0f;
	DWORD m_dwlastTime = 0;

	TGraphicImageInstanceMap m_GraphicImageInstanceMap;

	XMFLOAT4X4 m_matWorld{};
	XMFLOAT4X4 m_matTranslation{};
	XMFLOAT3 m_v3Position{};
	float m_fScaleX = 1.0f;
	float m_fScaleY = 1.0f;
	float m_fScaleZ = 1.0f;
	unsigned char m_ucRenderMode = SKY_RENDER_MODE_DEFAULT;
	std::string m_strCurTime;
	bool m_bTransitionStarted = false;
	bool m_bSkyMatrixUpdated = false;
	CGraphicImageInstance m_CloudAlphaImageInstance;
};

class CSkyBox : public CSkyObject
{
public:
	CSkyBox();
	~CSkyBox() override;

	void Update() override;
	void Update(const RenderContext& ctx) override;
	void Render() override;
	void Render(const RenderContext& ctx) override;
	void RenderCloud();
	void RenderCloud(const RenderContext& ctx);
	void Destroy() override;
	void Unload();

	void SetSkyBoxScale(const XMFLOAT3& scale);
	void SetGradientLevel(BYTE upper, BYTE lower);
	void SetFaceTexture(const char* filename, int faceIndex);
	void SetCloudTexture(const char* filename);
	void SetCloudScale(const XMFLOAT2& cloudScale);
	void SetCloudHeight(float height);
	void SetCloudTextureScale(const XMFLOAT2& cloudTextureScale);
	void SetCloudScrollSpeed(const XMFLOAT2& cloudScrollSpeed);
	void SetCloudColor(const TGradientColor& color, const TGradientColor& nextColor, const DWORD& transitionTime);
	void Refresh();
	void SetSkyColor(const TVectorGradientColor& colorVector, const TVectorGradientColor& nextColorVector, long transitionTime);
	void StartTransition() override;

private:
	void SetSkyObjectQuadVertical(TSkyObjectQuadVector* quads, const XMFLOAT2* points);
	void SetSkyObjectQuadHorizon(TSkyObjectQuadVector* quads, const XMFLOAT3* points);
	void SetQuadColor(CSkyObjectQuad& quad, unsigned char vertexIndex, const TColor& color, const TColor& nextColor, DWORD transitionTime);
	CGraphicImageInstance* FindTexture(const std::string& filename) const;

private:
	unsigned char m_ucVirticalGradientLevelUpper = 0;
	unsigned char m_ucVirticalGradientLevelLower = 0;
	std::array<TSkyObjectFace, 6> m_Faces{};
};
