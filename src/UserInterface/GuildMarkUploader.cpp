#include "StdAfx.h"
#include "GuildMarkUploader.h"
#include "Packet.h"

#include <DirectXTex/DirectXTex.h>

#ifdef __VTUNE__
#else

static bool LoadImageBGRA32(
	const char* fileName,
	DirectX::ScratchImage& outImage,
	DirectX::TexMetadata& outMetadata)
{
	if (!fileName || !fileName[0])
		return false;

	wchar_t wideFileName[MAX_PATH] = {};

	if (MultiByteToWideChar(CP_ACP, 0, fileName, -1, wideFileName, MAX_PATH) <= 0)
		return false;

	HRESULT hr = DirectX::LoadFromTGAFile(
		wideFileName,
		&outMetadata,
		outImage);

	if (FAILED(hr))
	{
		hr = DirectX::LoadFromWICFile(
			wideFileName,
			DirectX::WIC_FLAGS_IGNORE_SRGB,
			&outMetadata,
			outImage);
	}

	if (FAILED(hr))
		return false;

	if (outMetadata.format == DXGI_FORMAT_B8G8R8A8_UNORM)
		return true;

	DirectX::ScratchImage convertedImage;

	hr = DirectX::Convert(
		outImage.GetImages(),
		outImage.GetImageCount(),
		outMetadata,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DirectX::TEX_FILTER_DEFAULT,
		DirectX::TEX_THRESHOLD_DEFAULT,
		convertedImage);

	if (FAILED(hr))
		return false;

	outImage = std::move(convertedImage);
	outMetadata = outImage.GetMetadata();

	return true;
}

CGuildMarkUploader::CGuildMarkUploader()
{
	SetRecvBufferSize(1024);
	SetSendBufferSize(1024);
	__Inialize();
}

CGuildMarkUploader::~CGuildMarkUploader()
{
	__OfflineState_Set();
}

void CGuildMarkUploader::Disconnect()
{
	__OfflineState_Set();
}

bool CGuildMarkUploader::IsCompleteUploading()
{
	return STATE_OFFLINE == m_eState;
}

bool CGuildMarkUploader::__Save(const char* c_szFileName)
{
	/*
	int width = CGuildMarkImage::WIDTH;
	int height = CGuildMarkImage::HEIGHT;
	std::vector<unsigned char> rgba(width * height * 4);

	for (int i = 0; i < width * height; ++i) {
		rgba[i * 4 + 0] = m_kMark.m_apxBuf[i * 4 + 2]; // R
		rgba[i * 4 + 1] = m_kMark.m_apxBuf[i * 4 + 1]; // G
		rgba[i * 4 + 2] = m_kMark.m_apxBuf[i * 4 + 0]; // B
		rgba[i * 4 + 3] = m_kMark.m_apxBuf[i * 4 + 3]; // A
	}

	// Save as PNG
	if (!stbi_write_png(c_szFileName, width, height, 4, rgba.data(), width * 4)) {
		return false;
	}

	return true;
	*/
	return true;
}

bool CGuildMarkUploader::__Load(const char* c_szFileName, UINT* peError)
{
	DirectX::ScratchImage image;
	DirectX::TexMetadata metadata;

	if (!LoadImageBGRA32(c_szFileName, image, metadata))
	{
		*peError = ERROR_LOAD;
		return false;
	}

	if (metadata.width != SGuildMark::WIDTH)
	{
		*peError = ERROR_WIDTH;
		return false;
	}

	if (metadata.height != SGuildMark::HEIGHT)
	{
		*peError = ERROR_HEIGHT;
		return false;
	}

	const DirectX::Image* source = image.GetImage(0, 0, 0);

	if (!source || !source->pixels)
	{
		*peError = ERROR_LOAD;
		return false;
	}

	for (UINT y = 0; y < SGuildMark::HEIGHT; ++y)
	{
		const uint8_t* src = source->pixels + y * source->rowPitch;
		uint8_t* dst = reinterpret_cast<uint8_t*>(m_kMark.m_apxBuf) + y * SGuildMark::WIDTH * 4;

		memcpy(dst, src, SGuildMark::WIDTH * 4);
	}

	return true;
}

bool CGuildMarkUploader::__LoadSymbol(const char* c_szFileName, UINT* peError)
{
	DirectX::ScratchImage image;
	DirectX::TexMetadata metadata;

	if (!LoadImageBGRA32(c_szFileName, image, metadata))
	{
		*peError = ERROR_LOAD;
		return false;
	}

	if (metadata.width != 64)
	{
		*peError = ERROR_WIDTH;
		return false;
	}

	if (metadata.height != 128)
	{
		*peError = ERROR_HEIGHT;
		return false;
	}

	FILE* file = fopen(c_szFileName, "rb");

	if (!file)
	{
		*peError = ERROR_LOAD;
		return false;
	}

	fseek(file, 0, SEEK_END);
	const long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (fileSize <= 0)
	{
		fclose(file);
		*peError = ERROR_LOAD;
		return false;
	}

	m_symbolBuf.resize(static_cast<size_t>(fileSize));
	fread(m_symbolBuf.data(), m_symbolBuf.size(), 1, file);
	fclose(file);

	m_dwSymbolCRC32 = GetFileCRC32(c_szFileName);
	return true;
}

bool CGuildMarkUploader::Connect(const CNetworkAddress& c_rkNetAddr, DWORD dwHandle, DWORD dwRandomKey, DWORD dwGuildID, const char* c_szFileName, UINT* peError)
{
	__OfflineState_Set();
	SetRecvBufferSize(1024);
	SetSendBufferSize(1024);

	if (!CNetworkStream::Connect(c_rkNetAddr))
	{
		*peError=ERROR_CONNECT;
		return false;
	}

	m_dwSendType=SEND_TYPE_MARK;
	m_dwHandle=dwHandle;
	m_dwRandomKey=dwRandomKey;
	m_dwGuildID=dwGuildID;

	if (!__Load(c_szFileName, peError))
		return false;
		
	//if (!__Save(CGraphicMarkInstance::GetImageFileName().c_str()))
	//	return false;
	//CGraphicMarkInstance::ReloadImageFile();
	return true;
}

bool CGuildMarkUploader::ConnectToSendSymbol(const CNetworkAddress& c_rkNetAddr, DWORD dwHandle, DWORD dwRandomKey, DWORD dwGuildID, const char* c_szFileName, UINT* peError)
{
	__OfflineState_Set();
	SetRecvBufferSize(1024);
	SetSendBufferSize(64*1024);

	if (!CNetworkStream::Connect(c_rkNetAddr))
	{
		*peError=ERROR_CONNECT;
		return false;
	}

	m_dwSendType=SEND_TYPE_SYMBOL;
	m_dwHandle=dwHandle;
	m_dwRandomKey=dwRandomKey;
	m_dwGuildID=dwGuildID;

	if (!__LoadSymbol(c_szFileName, peError))
		return false;

	return true;
}

void CGuildMarkUploader::Process()
{
	CNetworkStream::Process();

	if (!__StateProcess())
	{
		__OfflineState_Set();
		Disconnect();
	}
}

void CGuildMarkUploader::OnConnectFailure()
{
	__OfflineState_Set();
}

void CGuildMarkUploader::OnConnectSuccess()
{
	__LoginState_Set();
}

void CGuildMarkUploader::OnRemoteDisconnect()
{
	__OfflineState_Set();
}

void CGuildMarkUploader::OnDisconnect()
{
	__OfflineState_Set();
}

void CGuildMarkUploader::__Inialize()
{
	m_eState = STATE_OFFLINE;

	m_dwGuildID = 0;
	m_dwHandle = 0;
	m_dwRandomKey = 0;

	m_symbolBuf.clear();
}

bool CGuildMarkUploader::__StateProcess()
{
	switch (m_eState)
	{
		case STATE_LOGIN:
			return __LoginState_Process();
			break;
	}

	return true;
}

void CGuildMarkUploader::__OfflineState_Set()
{
	__Inialize();
}

void CGuildMarkUploader::__CompleteState_Set()
{
	m_eState=STATE_COMPLETE;

	__OfflineState_Set();
}


void CGuildMarkUploader::__LoginState_Set()
{
	m_eState=STATE_LOGIN;
}

bool CGuildMarkUploader::__LoginState_Process()
{
	if (!__AnalyzePacket(GC::PHASE, sizeof(TPacketGCPhase), &CGuildMarkUploader::__LoginState_RecvPhase))
		return false;

	if (!__AnalyzePacket(GC::PING, sizeof(TPacketGCPing), &CGuildMarkUploader::__LoginState_RecvPingBase))
		return false;

	if (!__AnalyzePacket(GC::KEY_CHALLENGE, sizeof(TPacketGCKeyChallenge), &CGuildMarkUploader::__LoginState_RecvKeyChallengeBase))
		return false;

	if (!__AnalyzePacket(GC::KEY_COMPLETE, sizeof(TPacketGCKeyComplete), &CGuildMarkUploader::__LoginState_RecvKeyCompleteAndLogin))
		return false;

	return true;
}

bool CGuildMarkUploader::__SendMarkPacket()
{
	TPacketCGMarkUpload kPacketMarkUpload;
	kPacketMarkUpload.header=CG::MARK_UPLOAD;
	kPacketMarkUpload.length = sizeof(kPacketMarkUpload);
	kPacketMarkUpload.gid=m_dwGuildID;

	assert(sizeof(kPacketMarkUpload.image) == sizeof(m_kMark.m_apxBuf));
	memcpy(kPacketMarkUpload.image, m_kMark.m_apxBuf, sizeof(kPacketMarkUpload.image));

	if (!Send(sizeof(kPacketMarkUpload), &kPacketMarkUpload))
		return false;

	return true;
}
bool CGuildMarkUploader::__SendSymbolPacket()
{
	if (m_symbolBuf.empty())
		return false;

	TPacketCGSymbolUpload kPacketSymbolUpload;
	kPacketSymbolUpload.header=CG::GUILD_SYMBOL_UPLOAD;
	kPacketSymbolUpload.handle=m_dwGuildID;
	kPacketSymbolUpload.length=sizeof(TPacketCGSymbolUpload) + static_cast<uint16_t>(m_symbolBuf.size());

	if (!Send(sizeof(TPacketCGSymbolUpload), &kPacketSymbolUpload))
		return false;
	if (!Send(static_cast<int>(m_symbolBuf.size()), m_symbolBuf.data()))
		return false;

#ifdef _DEBUG
	printf("__SendSymbolPacket : [GuildID:%d/PacketSize:%d/BufSize:%d/CRC:%d]\n", m_dwGuildID, kPacketSymbolUpload.length, (int)m_symbolBuf.size(), m_dwSymbolCRC32);
#endif

	CNetworkStream::__SendInternalBuffer();
	__CompleteState_Set();

	return true;
}

bool CGuildMarkUploader::__LoginState_RecvPhase()
{
	TPacketGCPhase kPacketPhase;
	if (!Recv(sizeof(kPacketPhase), &kPacketPhase))
		return false;

	if (kPacketPhase.phase==PHASE_LOGIN)
	{
		if (SEND_TYPE_MARK == m_dwSendType)
		{
			if (!__SendMarkPacket())
				return false;
		}
		else if (SEND_TYPE_SYMBOL == m_dwSendType)
		{
			if (!__SendSymbolPacket())
				return false;
		}
	}

	return true;
}

// Ping/pong and key challenge now handled by CNetworkStream base class.
// Thin wrappers in the header delegate __AnalyzePacket dispatch to those base methods.

// RecvKeyComplete + mark-specific login authentication
bool CGuildMarkUploader::__LoginState_RecvKeyCompleteAndLogin()
{
	if (!CNetworkStream::RecvKeyComplete())
		return false;

	// Send mark login (authentication) now that secure channel is established
	TPacketCGMarkLogin kPacketMarkLogin;
	kPacketMarkLogin.header = CG::MARK_LOGIN;
	kPacketMarkLogin.length = sizeof(kPacketMarkLogin);
	kPacketMarkLogin.handle = m_dwHandle;
	kPacketMarkLogin.random_key = m_dwRandomKey;

	if (!Send(sizeof(kPacketMarkLogin), &kPacketMarkLogin))
		return false;

	return true;
}

bool CGuildMarkUploader::__AnalyzePacket(UINT uHeader, UINT uPacketSize, bool (CGuildMarkUploader::*pfnDispatchPacket)())
{
	uint16_t wHeader;
	if (!Peek(sizeof(wHeader), &wHeader))
		return true;

	if (wHeader != uHeader)
		return true;

	if (!Peek(uPacketSize))
		return true;

	return (this->*pfnDispatchPacket)();
}
#endif
