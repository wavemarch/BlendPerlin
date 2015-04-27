#include "LightHelper.fx"

struct VsIn {
	float3 posL : POSITION;
	float3 normalL : NORMAL;
	float2 tex : TEXCOORD;
};

struct VsOut {
	float4 posH : SV_POSITION;
	float4 ambient : COLOR0;
	float4 diffuse : COLOR1;
	float4 specular : COLOR2;
	float2 tex : TEXCOORD;
};

SamplerState spl {
	Filter = ANISOTROPIC;
	MaxAnisotropy = 4;

	AddressU = WRAP;
	AddressV = WRAP;
};


Texture2D gDiffuseMap;

cbuffer perObj {
	float4x4 gWorld;
	float4x4 gWorldViewProj;
	Material gMaterial;
	DirectionalLight dLight;
	float4 gEyePosW;
	float4x4 gTexTran;
};

VsOut VS(VsIn vin) {
	VsOut vout;
	
	vout.posH = mul(float4(vin.posL, 1.0f), gWorldViewProj);
	vout.tex = vin.tex;

	float3 normalW = mul(vin.normalL, (float3x3)gWorld);
	float3 posW = (float3)mul(float4(vin.posL, 1.0f), gWorld);

	float3 toEye = normalize((float3)gEyePosW - posW);

	ComputeDirectionalLight(gMaterial, dLight, normalW, toEye, vout.ambient, vout.diffuse, vout.specular);

	return vout;
}

float4 PS(VsOut vout) : SV_TARGET{
	float2 texcor = (float2)mul(float4(vout.tex, 0.0f, 1.0f), gTexTran);
	float4 txColor = gDiffuseMap.Sample(spl, texcor);

	float4 finalColor = 2.0f * txColor * (vout.ambient + vout.diffuse) + vout.specular;

	finalColor.a = txColor.a * gMaterial.Diffuse.a;
	
	return finalColor;
}


technique11 T0 {
	pass P0 {
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
};