#pragma once
#include "d3dApp.h"
#include "LightHelper.h"
#include "PerlinHeightMap.h"

class BlendPerlin :	public D3DApp {

	ID3DX11Effect *mFx;
	ID3DX11EffectTechnique *mTech;
	ID3DX11EffectPass *mPass;

	ID3DX11EffectShaderResourceVariable *fxTexture;
	ID3D11ShaderResourceView *mGrassTexture;
	ID3D11ShaderResourceView *mWaterTexture;

	ID3DX11EffectMatrixVariable *fxWorld;
	XMFLOAT4X4 mWorld;

	ID3DX11EffectMatrixVariable *fxWorldViewProj;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	ID3DX11EffectVariable *fxMaterial;
	Material mGrassMat;
	Material mWaterMat;

	ID3DX11EffectVariable *fxDlight;
	DirectionalLight mDlight;

	ID3DX11EffectVectorVariable *fxEyePosW;
	XMFLOAT4 mEyePosW;

	ID3DX11EffectMatrixVariable *fxTexTran;
	XMFLOAT4X4 mTexTran;


	ID3D11Buffer *mHillsBuf;
	ID3D11Buffer *mHillsIndexBuf;
	PerlinHeightMap mHills;

	ID3D11Buffer *mWaveBuf;
	ID3D11Buffer *mWaveIndexBuf;
	PerlinHeightMap mWaves;
	float mWaveUpdateAccumulateTime;

	ID3D11InputLayout *mLayout;

	ID3D11RasterizerState *mRsState;


	POINT mLastMoustPoint;
	float YawFromZfront;
	float Pitch;


	void CompileEffect();
	void LoadTextures();
	void BuildHillsMesh();
	void BuildWavesMesh();
	void RefreshWavesMesh();
	void CreateInputLayout();

public:
	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

	BlendPerlin(HINSTANCE hInstanc3);
	virtual ~BlendPerlin();
};

