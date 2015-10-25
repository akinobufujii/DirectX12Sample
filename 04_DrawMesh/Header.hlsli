#ifndef __AKI_HLSL_H__
#define __AKI_HLSL_H__

cbuffer cbMatrix : register(b0)
{
	matrix	_WVP;
};

// ���_���͍\����
struct VS_IN 
{
	float4 pos		: POSITION;	// ���_���W
	float4 color	: COLOR;	// ���_�J���[
	float2 uv		: TEXCOORD;	// UV���W
};

// ���_�o�͍\����
struct VS_OUT
{
	float4 pos		: SV_POSITION;	// ���_���W
	float4 color	: COLOR;		// ���_�J���[
	float2 uv		: TEXCOORD;	// UV���W
};

#endif	// __AKI_HLSL_H__
