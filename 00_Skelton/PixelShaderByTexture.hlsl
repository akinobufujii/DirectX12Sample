#include "Header.hlsli"

Texture2D		g_Tex		: register(t0);	// �e�N�X�`���[
SamplerState	g_Sampler	: register(s0);	// �T���v���[�X�e�[�g

float4 main(VS_OUT vsin) : SV_TARGET
{
	return g_Tex.Sample(g_Sampler, vsin.uv);
}
