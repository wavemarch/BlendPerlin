#include "BlendPerlin.h"


BlendPerlin::BlendPerlin(HINSTANCE hInstance) : 
D3DApp(hInstance),
mFx(NULL),
mTech(NULL),
mPass(NULL),
fxTexture(NULL),
mGrassTexture(NULL),
mWaterTexture(NULL),
fxWorld(NULL),
fxWorldViewProj(NULL),
fxMaterial(NULL),
fxDlight(NULL),
fxEyePosW(NULL),
fxTexTran(NULL),
mHillsBuf(NULL),
mHillsIndexBuf(NULL),
mWaveBuf(NULL),
mWaveIndexBuf(NULL),
mLayout(NULL)
{
	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	mLastMoustPoint.x = 0;
	mLastMoustPoint.y = 0;

	mEyePosW = XMFLOAT4(0, 0, 0, 1);
	YawFromZfront = 0;
	Pitch = 0;

	mGrassMat.Ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	mGrassMat.Diffuse = XMFLOAT4(0.48f, 0.77f, 0.46, 1.0f);
	mGrassMat.Specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 18.0f);

	mDlight.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDlight.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDlight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDlight.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	mDlight.Pad = 1.0f;

	XMStoreFloat4x4(&mTexTran, I);
	mWaveUpdateAccumulateTime = 0.0f;
}

bool BlendPerlin::Init() {
	if (!D3DApp::Init())
		return false;

	CompileEffect();
	LoadTextures();
	BuildHillsMesh();
	BuildWavesMesh();
	CreateInputLayout();

	D3D11_RASTERIZER_DESC rds;
	ZeroMemory(&rds, sizeof(D3D11_RASTERIZER_DESC));
	rds.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	rds.DepthClipEnable = true;
	rds.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;

	HR(md3dDevice->CreateRasterizerState(&rds, &mRsState));


	D3D11_BLEND_DESC bsc;
	ZeroMemory(&bsc, sizeof(D3D11_BLEND_DESC));
	bsc.AlphaToCoverageEnable = false;
	bsc.IndependentBlendEnable = false;
	bsc.RenderTarget[0].BlendEnable = TRUE;
	bsc.RenderTarget[0].BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	bsc.RenderTarget[0].SrcBlend = D3D11_BLEND::D3D11_BLEND_BLEND_FACTOR;
	bsc.RenderTarget[0].DestBlend = D3D11_BLEND::D3D11_BLEND_INV_BLEND_FACTOR;
	bsc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	bsc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
	bsc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;
	bsc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;

	HR(md3dDevice->CreateBlendState(&bsc, &mTransprancyBlendState));

	bsc.RenderTarget[0].BlendEnable = FALSE;
	HR(md3dDevice->CreateBlendState(&bsc, &mNotBlendState));

	return true;
}

void BlendPerlin::CompileEffect() {
	DWORD shaderFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	ID3D10Blob *shaderBinary = NULL;
	ID3D10Blob *errMsg = NULL;

	HRESULT hr = D3DX11CompileFromFile(L"Color.fx", NULL, NULL, NULL, "fx_5_0", shaderFlags, NULL, NULL, &shaderBinary, &errMsg, NULL);

	if (errMsg != NULL){
		MessageBoxA(NULL, (char*)errMsg->GetBufferPointer(), NULL, NULL);
		ReleaseCOM(errMsg);
	}

	if (FAILED(hr)){
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX11CompileFromFile", true);
	}

	HR(D3DX11CreateEffectFromMemory(shaderBinary->GetBufferPointer(), shaderBinary->GetBufferSize(), shaderFlags, md3dDevice, &mFx));
	mTech = mFx->GetTechniqueByName("T0");
	mPass = mTech->GetPassByName("P0");

	fxTexture = mFx->GetVariableByName("gDiffuseMap")->AsShaderResource();
	fxWorld = mFx->GetVariableByName("gWorld")->AsMatrix();
	fxWorldViewProj = mFx->GetVariableByName("gWorldViewProj")->AsMatrix();
	fxMaterial = mFx->GetVariableByName("gMaterial");
	fxDlight = mFx->GetVariableByName("dLight");
	fxEyePosW = mFx->GetVariableByName("gEyePosW")->AsVector();
	fxTexTran = mFx->GetVariableByName("gTexTran")->AsMatrix();

	ReleaseCOM(shaderBinary);

}

void BlendPerlin::LoadTextures(){
	HR(D3DX11CreateShaderResourceViewFromFile(md3dDevice, L"skitch.png", NULL, NULL, &mGrassTexture, NULL));
	HR(D3DX11CreateShaderResourceViewFromFile(md3dDevice, L"waves.jpg", NULL, NULL, &mWaterTexture, NULL));
}

void BlendPerlin::BuildHillsMesh(){
	mHills.Refresh(5, 5, 20);

	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(D3D11_BUFFER_DESC));
	vbd.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = mHills.mapHeight * mHills.mapWidth * sizeof(vertex);
	vbd.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA vData;
	ZeroMemory(&vData, sizeof(D3D11_SUBRESOURCE_DATA));
	vData.pSysMem = mHills.vData;
	
	HR(md3dDevice->CreateBuffer(&vbd, &vData, &mHillsBuf));


	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(D3D11_BUFFER_DESC));
	ibd.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;
	ibd.ByteWidth = mHills.M * sizeof(UINT);
	ibd.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA iData;
	iData.pSysMem = mHills.iData;

	HR(md3dDevice->CreateBuffer(&ibd, &iData, &mHillsIndexBuf));
}

void BlendPerlin::BuildWavesMesh(){
	mWaves.Refresh(10, 10, 10);
	
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(D3D11_BUFFER_DESC));
	vbd.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = mWaves.mapHeight * mWaves.mapWidth * sizeof(vertex);
	vbd.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA vData;
	ZeroMemory(&vData, sizeof(D3D11_SUBRESOURCE_DATA));
	vData.pSysMem = mWaves.vData;

	HR(md3dDevice->CreateBuffer(&vbd, &vData, &mWaveBuf));

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(D3D11_BUFFER_DESC));
	ibd.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;
	ibd.ByteWidth = mWaves.M * sizeof(UINT);
	ibd.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA iData;
	ZeroMemory(&iData, sizeof(D3D11_SUBRESOURCE_DATA));
	iData.pSysMem = mWaves.iData;

	HR(md3dDevice->CreateBuffer(&ibd, &iData, &mWaveIndexBuf));
}

void BlendPerlin::RefreshWavesMesh(){
	mWaves.Refresh();

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(md3dImmediateContext->Map(mWaveBuf, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy(mappedData.pData, mWaves.vData, sizeof(vertex)* mWaves.mapHeight * mWaves.mapWidth);
	md3dImmediateContext->Unmap(mWaveBuf, 0);
}

void BlendPerlin::CreateInputLayout(){
	D3D11_INPUT_ELEMENT_DESC inputDecs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	D3DX11_PASS_DESC pDecs;
	HR(mPass->GetDesc(&pDecs));

	HR(md3dDevice->CreateInputLayout(inputDecs, 3, pDecs.pIAInputSignature, pDecs.IAInputSignatureSize, &mLayout));
}

void BlendPerlin::OnResize(){
	D3DApp::OnResize();

	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(0.25 * XM_PI, AspectRatio(), 1.0f, 9000.0f);
	XMStoreFloat4x4(&mProj, perspectiveMatrix);
}

void BlendPerlin::OnMouseDown(WPARAM btnState, int x , int y) {
	mLastMoustPoint.x = x;
	mLastMoustPoint.y = y;
	SetCapture(mhMainWnd);
}

void BlendPerlin::OnMouseMove(WPARAM btnState, int x, int y) {
	const int dx = x - mLastMoustPoint.x;
	const int dy = y - mLastMoustPoint.y;

	if (btnState & MK_LBUTTON) {
		float factor = 1.0f;
		
		YawFromZfront += XMConvertToRadians((dx)* factor);;
		Pitch += XMConvertToRadians((-dy) * factor);

		Pitch = MathHelper::Clamp(Pitch, 0.1f-XM_PI, XM_PI - 0.1f);

		float tgtX = sinf(YawFromZfront);
		float tgtZ = cosf(YawFromZfront);
		float tgtY = sinf(Pitch);

		XMVECTOR eyePos = XMLoadFloat4(&mEyePosW);
		XMMATRIX viewMatrix = XMMatrixLookAtLH(eyePos, eyePos + XMVectorSet(tgtX, tgtY, tgtZ, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		XMStoreFloat4x4(&mView, viewMatrix);
	}
	else if (btnState & MK_RBUTTON){
		float d = (dy - dx) * 0.2f;

		float tgtX = sinf(YawFromZfront);
		float tgtZ = cosf(YawFromZfront);
		float tgtY = sinf(Pitch);

		XMVECTOR lookVector = XMVectorSet(tgtX, tgtY, tgtZ, 0.0f);
		XMVECTOR stepVector = lookVector*d;

		XMVECTOR eyePos = XMLoadFloat4(&mEyePosW) + stepVector;
		XMStoreFloat4(&mEyePosW, eyePos);

		XMMATRIX viewMatrix = XMMatrixLookAtLH(eyePos, eyePos + lookVector, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		XMStoreFloat4x4(&mView, viewMatrix);
	}

	mLastMoustPoint.x = x;
	mLastMoustPoint.y = y;
}

void BlendPerlin::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void BlendPerlin::UpdateScene(float dt){
	float tgtX = sinf(YawFromZfront);
	float tgtZ = cosf(YawFromZfront);
	float tgtY = sinf(Pitch);

	XMVECTOR eyePos = XMLoadFloat4(&mEyePosW);
	XMMATRIX viewMatrix = XMMatrixLookAtLH(eyePos, eyePos + XMVectorSet(tgtX, tgtY, tgtZ, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	XMStoreFloat4x4(&mView, viewMatrix);

		
	mWaveUpdateAccumulateTime += dt;

	if (mWaveUpdateAccumulateTime >= 0.08f){
		mWaveUpdateAccumulateTime = 0.0f;
		RefreshWavesMesh();
	}
}

void BlendPerlin::DrawScene(){
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, (float*)&Colors::LightSteelBlue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

//	md3dImmediateContext->RSSetState(mRsState);

	float TransMask[4] = {0.7f, 0.7f, 0.7f, 1.0f};
	md3dImmediateContext->OMSetBlendState(mNotBlendState, TransMask, 0xffffffff);

	UINT strides = sizeof(vertex);
	UINT offsets = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &(mHillsBuf), &strides, &offsets);
	md3dImmediateContext->IASetIndexBuffer(mHillsIndexBuf, DXGI_FORMAT_R32_UINT, 0);

	fxTexture->SetResource(mGrassTexture);
	fxWorld->SetMatrix((float*)&mWorld);

	XMMATRIX simdWorld = XMLoadFloat4x4(&mWorld);
	XMMATRIX simdView = XMLoadFloat4x4(&mView);
	XMMATRIX simdProj = XMLoadFloat4x4(&mProj);
	XMMATRIX simdWvp = simdWorld * simdView * simdProj;
	fxWorldViewProj->SetMatrix((float*)&(simdWvp));

	fxMaterial->SetRawValue(&mGrassMat, 0, sizeof(mGrassMat));
	fxDlight->SetRawValue(&mDlight, 0, sizeof(mDlight));
	fxEyePosW->SetFloatVector((float*)&mEyePosW);
	fxTexTran->SetMatrix((float*)&mTexTran);

	mPass->Apply(0, md3dImmediateContext);
	md3dImmediateContext->DrawIndexed(mHills.M, 0, 0);

	md3dImmediateContext->IASetVertexBuffers(0, 1, &(mWaveBuf), &strides, &offsets);
	md3dImmediateContext->IASetIndexBuffer(mWaveIndexBuf, DXGI_FORMAT_R32_UINT, 0);
	fxTexture->SetResource(mWaterTexture);
	md3dImmediateContext->OMSetBlendState(mTransprancyBlendState, TransMask, 0xffffffff);
	mPass->Apply(0, md3dImmediateContext);
	md3dImmediateContext->DrawIndexed(mWaves.M, 0, 0);

	HR(mSwapChain->Present(0, 0));
}

BlendPerlin::~BlendPerlin() {
	ReleaseCOM(mFx);
	ReleaseCOM(mGrassTexture);
	ReleaseCOM(mWaterTexture);
	ReleaseCOM(mHillsBuf);
	ReleaseCOM(mHillsIndexBuf);
	ReleaseCOM(mWaveBuf);
	ReleaseCOM(mWaveIndexBuf);
	ReleaseCOM(mLayout);
	ReleaseCOM(mRsState);
	ReleaseCOM(mTransprancyBlendState);
	ReleaseCOM(mNotBlendState);
}
