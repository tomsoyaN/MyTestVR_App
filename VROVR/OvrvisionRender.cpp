#include "OvrvisionRender.h"

VECTOR2::VECTOR2(float a, float b) {
	u = a; v = b;
}
VECTOR3::VECTOR3(float a, float b, float c) {
	x = a; y = b; z = c;
}

int OvrvisionRender::InitializeCamPlane(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int w, int h, float zsize) {
	ID3DBlob *pCompiledShader = nullptr;
	ID3DBlob *pErrors = nullptr;

	if (FAILED(D3DX11CompileFromMemory(g_planeShader, strlen(g_planeShader), NULL, NULL, NULL, "VSFunc", "vs_4_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, NULL, &pCompiledShader, &pErrors, NULL)))
	{
		return E_FAIL;
	}
	Release(pErrors);

	if (FAILED(device->CreateVertexShader(pCompiledShader->GetBufferPointer(), pCompiledShader->GetBufferSize(), NULL, &VertexShader)))
	{
		Release(pCompiledShader);
		return E_FAIL;
	}

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = sizeof(layout) / sizeof(layout[0]);

	if (FAILED(device->CreateInputLayout(layout, numElements, pCompiledShader->GetBufferPointer(), pCompiledShader->GetBufferSize(), &VertexLayout)))
		return FALSE;

	deviceContext->IASetInputLayout(VertexLayout);

	if (FAILED(D3DX11CompileFromMemory(g_planeShader, strlen(g_planeShader), NULL, NULL, NULL, "PSFunc", "ps_4_0", 0, 0, NULL, &pCompiledShader, &pErrors, NULL)))
	{
		return E_FAIL;
	}
	Release(pErrors);
	if (FAILED(device->CreatePixelShader(pCompiledShader->GetBufferPointer(), pCompiledShader->GetBufferSize(), NULL, &PixelShader)))
	{
		Release(pCompiledShader);
		return E_FAIL;
	}
	Release(pCompiledShader);

	float aspect = (float)h / (float)w * 0.82f;
	SimpleVertex vertices[] =
	{
		VECTOR3(-zsize, -zsize * aspect, 0.5f), VECTOR2(0, 1),
		VECTOR3(-zsize, zsize*aspect, 0.5f), VECTOR2(0, 0),
		VECTOR3(zsize, -zsize * aspect, 0.5f), VECTOR2(1, 1),
		VECTOR3(zsize, zsize*aspect, 0.5f), VECTOR2(1, 0),
	};
	D3D11_BUFFER_DESC bd;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = vertices;

	if (FAILED(device->CreateBuffer(&bd, &InitData, &VertexBuffer)))
		return FALSE;

	// Set Buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//Sampler
	D3D11_SAMPLER_DESC SamDesc;
	ZeroMemory(&SamDesc, sizeof(D3D11_SAMPLER_DESC));

	SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	device->CreateSamplerState(&SamDesc, &SampleLinear);

	//Create texture
	D3D11_TEXTURE2D_DESC texDesc;
	memset(&texDesc, 0, sizeof(texDesc));

	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.Width = w;
	texDesc.Height = h;
	texDesc.CPUAccessFlags = 0;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;

	if (FAILED(device->CreateTexture2D(&texDesc, NULL, &Texture2d)))
	{
		return E_FAIL;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	memset(&srvDesc, 0, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	if (FAILED(device->CreateShaderResourceView(Texture2d, &srvDesc, &ShaderRC))) {
		return E_FAIL;
	}

	return S_OK;
}

int OvrvisionRender::CleanCamPlane() {
	Release(SampleLinear);
	Release(Texture2d);
	Release(ShaderRC);
	Release(VertexShader);
	Release(PixelShader);
	Release(VertexBuffer);
	Release(VertexLayout);

	return S_OK;
}

int OvrvisionRender::SetCamImage(ID3D11DeviceContext* DeviceContext, unsigned char* camImage, unsigned int imageRowsize) {
	if (Texture2d == NULL) return S_FALSE;
	D3D11_TEXTURE2D_DESC desc;
	Texture2d->GetDesc(&desc);
	
	DeviceContext->UpdateSubresource(Texture2d, 0, NULL, camImage, imageRowsize, 0);
	return S_OK;
}

int OvrvisionRender::RendererCamPlane(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
{
	if (Texture2d == NULL)
		return S_FALSE;

	DeviceContext->VSSetShader(VertexShader, NULL, 0);
	DeviceContext->PSSetShader(PixelShader, NULL, 0);

	DeviceContext->PSSetSamplers(0, 1, &SampleLinear);
	DeviceContext->PSSetShaderResources(0, 1, &ShaderRC);

	DeviceContext->Draw(4, 0);

	return S_OK;
}