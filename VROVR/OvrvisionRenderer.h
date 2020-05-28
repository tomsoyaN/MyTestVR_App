#pragma once
#include "Win32_DirectXAppUtil.h"
#include <D3DX11async.h>

#pragma comment(lib,"d3dx11.lib")

class VECTOR2 {
public:
	VECTOR2(float, float);
	float u;
	float v;
};
class VECTOR3{
public:
	VECTOR3(float, float, float);
	float x;
	float y;
	float z;
};
struct SimpleVertex {
	VECTOR3 Pos;
	VECTOR2 Uv;
};
struct OvrvisionRender {
	LPCSTR g_planeShader =
		"Texture2D g_texDecal : register(ps,t0);\n"
		"SamplerState g_samLinear : register(ps,s0);\n"
		"\n"
		"   struct VS_OUTPUT\n"
		"   {\n"
		"      float4 Pos : SV_POSITION;\n"
		"      float2 Tex : TEXCOORD;\n"
		"   };\n"
		"\n"
		"   VS_OUTPUT VSFunc(float4 Pos : POSITION, float2 Tex : TEXCOORD)\n"
		"   {\n"
		"      VS_OUTPUT output = (VS_OUTPUT)0;\n"
		"      output.Pos = Pos;\n"
		"      output.Tex = Tex;\n"
		"      return output;\n"
		"   }\n"
		"\n"
		"   float4 PSFunc(VS_OUTPUT input) : SV_Target\n"
		"   {\n"
		"      return g_texDecal.Sample(g_samLinear, input.Tex);\n"
		"   }\n"
		"\n"
		;
	ID3D11InputLayout* VertexLayout = NULL;
	ID3D11Buffer* VertexBuffer = NULL;
	ID3D11VertexShader* VertexShader = NULL;
	ID3D11PixelShader* PixelShader = NULL;
	ID3D11SamplerState* SampleLinear = NULL;
	ID3D11Texture2D* Texture2d = NULL;
	ID3D11ShaderResourceView* ShaderRC = NULL;

	int InitializeCamPlane(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int w, int h, float zsize);
	int CleanCamPlane();
	int SetCamImage(ID3D11DeviceContext* deviceContext, unsigned char* camImage, unsigned int imageRowsize);
	int RendererCamPlane(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
};
static OvrvisionRender OvrvisionRenderer; //global OvrvisionRender State