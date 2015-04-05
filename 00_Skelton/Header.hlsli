#ifndef __AKI_HLSL_H__
#define __AKI_HLSL_H__

// ���_���͍\����
struct VS_IN 
{
	float4 pos		: POSITION;	// ���_���W
	float4 color	: COLOR;	// ���_�J���[
};

// ���_�o�͍\����
struct VS_OUT
{
	float4 pos		: SV_POSITION;	// ���_���W
	float4 color	: COLOR;		// ���_�J���[
};

#endif	// __AKI_HLSL_H__
