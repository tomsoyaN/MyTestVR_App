#pragma once
#include <ovrvision_pro.h>
#include "OVR_CAPI_D3D.h"
#include "Win32_DirectXAppUtil.h"

struct OculusTexture {
	ovrSession Session;
	ovrTextureSwapChain TextureChain;
	ovrTextureSwapChain DepthTextureChain;
	std::vector<ID3D11RenderTargetView*> TexRtv;
	std::vector<ID3D11DepthStencilView*> TexDsv;

	OculusTexture() : Session(nullptr), TextureChain(nullptr), DepthTextureChain(nullptr) {};
	bool Init(ovrSession session, ID3D11Device *device,int sizeW, int sizeH, int sampleCnt, bool createDepth);
	~OculusTexture();
	ID3D11RenderTargetView * GetRTV();
	ID3D11DepthStencilView * GetDSV();
	void Commit();
};
