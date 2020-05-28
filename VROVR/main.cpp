#include <Win32_DirectXAppUtil.h>
#include "OVR_CAPI_D3D.h"
#include <ovrvision_pro.h>
#include"OculusTexture.h"
#include "OvrvisionRenderer.h"

static bool MainLoop(bool retrycreate) {
	ovrMirrorTexture mirrorTexture = nullptr;
	OculusTexture * pEyeRenderTexture[2] = { nullptr,nullptr };
	Camera * mainCam = nullptr;
	Scene          * roomScene = nullptr;
	ovrMirrorTextureDesc mirrorDesc = {};
	long long frameIndex = 0;
	int msaaRate = 4;
	ovrSessionStatus sessionStatus;
	ovrSession session;
	ovrGraphicsLuid luid;
	OVR::OvrvisionPro ovrvision;
	int width = 0, height = 0, pixelsize = 4;

	ovrResult result = ovr_Create(&session, &luid);
	
	if (!OVR_SUCCESS(result)) return retrycreate;

	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);

	if (!DIRECTX.InitDevice(hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h, reinterpret_cast<LUID*>(&luid))) goto Done;
	
	ovrRecti eyeRenderViewport[2];
	for (int eye = 0; eye < 2; ++eye) {

		ovrSizei idealSize = ovr_GetFovTextureSize(session, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
		pEyeRenderTexture[eye] = new OculusTexture();
		if (!pEyeRenderTexture[eye]->Init(session, DIRECTX.Device, idealSize.w, idealSize.h, msaaRate, true)) {
			if (retrycreate) goto Done;
		}
		eyeRenderViewport[eye].Pos.x = 0;
		eyeRenderViewport[eye].Pos.y = 0;
		eyeRenderViewport[eye].Size = idealSize;
		if (!pEyeRenderTexture[eye]->TextureChain || !pEyeRenderTexture[eye]->DepthTextureChain) {
			if (retrycreate) goto Done;
		}
	
	}
	
	mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	mirrorDesc.Width = DIRECTX.WinSizeW;
	mirrorDesc.Height = DIRECTX.WinSizeH;
	mirrorDesc.MirrorOptions = ovrMirrorOption_Default;
	result = ovr_CreateMirrorTextureWithOptionsDX(session, DIRECTX.Device, &mirrorDesc, &mirrorTexture);
	
	if (!OVR_SUCCESS(result))
	{
		if (retrycreate) goto Done;
	}

	mainCam = new Camera(XMVectorSet(0.0f, 0.0f, 5.0f, 0), XMQuaternionIdentity());
	
	int locationID = 0;
	OVR::Camprop cameraMode = OVR::OV_CAMVR_FULL;
	if (ovrvision.Open(locationID, cameraMode)) {
		width = ovrvision.GetCamWidth();
		height = ovrvision.GetCamHeight();
		pixelsize = ovrvision.GetCamPixelsize();

		ovrvision.SetCameraSyncMode(false);
		OvrvisionRenderer.InitializeCamPlane(DIRECTX.Device, DIRECTX.Context, width, height, 1.0f);
		result = ovr_GetSessionStatus(session, &sessionStatus);
	}
	

	while (DIRECTX.HandleMessages()) {
		//ovrSessionStatus sessionStatus;
		result = ovr_GetSessionStatus(session, &sessionStatus);
		if (true) {
			ovrEyeRenderDesc eyeRenderDesc[2];
			eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
			eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);


			ovrPosef EyeRenderPose[2];
			ovrPosef HmdToEyePose[2] = { eyeRenderDesc[0].HmdToEyePose,eyeRenderDesc[1].HmdToEyePose };

			double sensorSampleTime;

			ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, EyeRenderPose, &sensorSampleTime);

			ovrTimewarpProjectionDesc posTimewarpProjectionDesc = {};

			ovrvision.PreStoreCamData(OVR::Camqt::OV_CAMQT_DMSRMP);

			for (int eye = 0; eye < 2; ++eye) {
				DIRECTX.SetAndClearRenderTarget(pEyeRenderTexture[eye]->GetRTV(), (ID3D11DepthStencilView*)nullptr);
				DIRECTX.SetViewport((float)eyeRenderViewport[eye].Pos.x, (float)eyeRenderViewport[eye].Pos.y,
					(float)eyeRenderViewport[eye].Size.w, (float)eyeRenderViewport[eye].Size.h);
				XMVECTOR eyeQuat = XMVectorSet(EyeRenderPose[eye].Orientation.x, EyeRenderPose[eye].Orientation.y,
					EyeRenderPose[eye].Orientation.z, EyeRenderPose[eye].Orientation.w);
				XMVECTOR eyePos = XMVectorSet(EyeRenderPose[eye].Position.x, EyeRenderPose[eye].Position.y, EyeRenderPose[eye].Position.z, 0);

				XMVECTOR CombinedPos = XMVectorAdd(mainCam->Pos, XMVector3Rotate(eyePos, mainCam->Rot));
				Camera finalCam(CombinedPos, XMQuaternionMultiply(eyeQuat, mainCam->Rot));
				XMMATRIX view = finalCam.GetViewMatrix();
				ovrMatrix4f p = ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, 0.2f, 1000.0f, ovrProjection_None);
				posTimewarpProjectionDesc = ovrTimewarpProjectionDesc_FromProjection(p, ovrProjection_None);

				XMMATRIX proj = XMMatrixSet(p.M[0][0], p.M[1][0], p.M[2][0], p.M[3][0],
					p.M[0][1], p.M[1][1], p.M[2][1], p.M[3][1],
					p.M[0][2], p.M[1][2], p.M[2][2], p.M[3][2],
					p.M[0][3], p.M[1][3], p.M[2][3], p.M[3][3]);
				XMMATRIX prod = XMMatrixMultiply(view, proj);

				pEyeRenderTexture[eye]->Commit();

				if (eye == 0)
					OvrvisionRenderer.SetCamImage(DIRECTX.Context, ovrvision.GetCamImageBGRA(OVR::Cameye::OV_CAMEYE_LEFT), width*pixelsize);
				else
					OvrvisionRenderer.SetCamImage(DIRECTX.Context, ovrvision.GetCamImageBGRA(OVR::Cameye::OV_CAMEYE_RIGHT), width*pixelsize);

				OvrvisionRenderer.RendererCamPlane(DIRECTX.Device, DIRECTX.Context);
			}

			ovrLayerEyeFovDepth ld = {};
			ld.Header.Type = ovrLayerType_EyeFovDepth;
			ld.Header.Flags = 0;
			ld.ProjectionDesc = posTimewarpProjectionDesc;
			ld.SensorSampleTime = sensorSampleTime;

			for (int eye = 0; eye < 2; ++eye) {
				ld.ColorTexture[eye] = pEyeRenderTexture[eye]->TextureChain;
				ld.DepthTexture[eye] = pEyeRenderTexture[eye]->DepthTextureChain;
				ld.Viewport[eye] = eyeRenderViewport[eye];
				ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
				ld.RenderPose[eye] = EyeRenderPose[eye];
			}
			ovrLayerHeader* layers = &ld.Header;
			result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);

			if (!OVR_SUCCESS(result))

				frameIndex++;
		}
		ID3D11Texture2D* tex = nullptr;
		ovr_GetMirrorTextureBufferDX(session, mirrorTexture, IID_PPV_ARGS(&tex));
		DIRECTX.Context->CopyResource(DIRECTX.BackBuffer, tex);
		tex->Release();
		DIRECTX.SwapChain->Present(0, 0);

	}
	Done:
		delete mainCam;
		if (mirrorTexture) ovr_DestroyMirrorTexture(session, mirrorTexture);
		for (int eye = 0; eye < 2; ++eye) {
			delete pEyeRenderTexture[eye];
		}
		ovrvision.Close();
		DIRECTX.ReleaseDevice();
		ovr_Destroy(session);
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR ,int) {

	ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
	ovrResult result = ovr_Initialize(&initParams);
	VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");
	DIRECTX.Run(MainLoop);
	ovr_Shutdown();
	return 0;

}