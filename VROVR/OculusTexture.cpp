#include"OculusTexture.h"

bool OculusTexture::Init(ovrSession session,ID3D11Device *device, int sizeW, int sizeH, int sampleCnt, bool createDepth) {
	Session = session;
	{
		ovrTextureSwapChainDesc desc = {};
		desc.Type = ovrTexture_2D;
		desc.ArraySize = 1;
		desc.Width = sizeW;
		desc.Height = sizeH;
		desc.MipLevels = 1;
		desc.SampleCount = sampleCnt;
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.MiscFlags = ovrTextureMisc_DX_Typeless | ovrTextureMisc_AutoGenerateMips;
		desc.BindFlags = ovrTextureBind_DX_RenderTarget;
		desc.StaticImage = ovrFalse;

		ovrResult result = ovr_CreateTextureSwapChainDX(session, device, &desc, &TextureChain);
		if (!OVR_SUCCESS(result)) {
			return false;
		}

		int textureCnt = 0;
		ovr_GetTextureSwapChainLength(Session, TextureChain, &textureCnt);

		for (int i = 0; i < textureCnt; ++i) {
			ID3D11Texture2D* tex = nullptr;
			ovr_GetTextureSwapChainBufferDX(Session, TextureChain, i, IID_PPV_ARGS(&tex));

			D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
			rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvd.ViewDimension = (sampleCnt > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS
				: D3D11_RTV_DIMENSION_TEXTURE2D;
			ID3D11RenderTargetView* rtv;
			HRESULT hr = device->CreateRenderTargetView(tex, &rtvd, &rtv);
			TexRtv.push_back(rtv);
			tex->Release();
		}
	}

		if (createDepth)
		{
			ovrTextureSwapChainDesc desc = {};
			desc.Type = ovrTexture_2D;
			desc.ArraySize = 1;
			desc.Width = sizeW;
			desc.Height = sizeH;
			desc.MipLevels = 1;
			desc.SampleCount = sampleCnt;
			desc.Format = OVR_FORMAT_D32_FLOAT;
			desc.MiscFlags = ovrTextureMisc_None;
			desc.BindFlags = ovrTextureBind_DX_DepthStencil;
			desc.StaticImage = ovrFalse;

			ovrResult result = ovr_CreateTextureSwapChainDX(session, device, &desc, &DepthTextureChain);
			if (!OVR_SUCCESS(result))
				return false;

			int textureCount = 0;
			ovr_GetTextureSwapChainLength(Session, DepthTextureChain, &textureCount);
			for (int i = 0; i < textureCount; ++i)
			{
				ID3D11Texture2D* tex = nullptr;
				ovr_GetTextureSwapChainBufferDX(Session, DepthTextureChain, i, IID_PPV_ARGS(&tex));

				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = (sampleCnt > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS
					: D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;

				ID3D11DepthStencilView* dsv;
				HRESULT hr = device->CreateDepthStencilView(tex, &dsvDesc, &dsv);
				VALIDATE((hr == ERROR_SUCCESS), "Error creating depth stencil view");
				TexDsv.push_back(dsv);
				tex->Release();
			}
		}
	
	return true;
}

OculusTexture::~OculusTexture()
{
	for (int i = 0; i < (int)TexRtv.size(); ++i)
	{
		Release(TexRtv[i]);
	}
	for (int i = 0; i < (int)TexDsv.size(); ++i)
	{
		Release(TexDsv[i]);
	}
	if (TextureChain)
	{
		ovr_DestroyTextureSwapChain(Session, TextureChain);
	}
	if (DepthTextureChain)
	{
		ovr_DestroyTextureSwapChain(Session, DepthTextureChain);
	}
}

ID3D11RenderTargetView* OculusTexture::GetRTV() {
	int index = 0;
	ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &index);
	return TexRtv[index];
}

ID3D11DepthStencilView* OculusTexture::GetDSV() {
	int index = 0;
	ovr_GetTextureSwapChainCurrentIndex(Session, DepthTextureChain, &index);
	return TexDsv[index];
}

void OculusTexture::Commit() {
	ovr_CommitTextureSwapChain(Session, TextureChain);
	ovr_CommitTextureSwapChain(Session, DepthTextureChain);
}

