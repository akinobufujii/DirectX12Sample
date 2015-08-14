#ifndef __AKI_HLSL_H__
#define __AKI_HLSL_H__

cbuffer cbMatrix : register(b0)
{
	matrix	_WVP;
};

// 頂点入力構造体
struct VS_IN 
{
	float4 pos		: POSITION;	// 頂点座標
	float4 color	: COLOR;	// 頂点カラー
	float2 uv		: TEXCOORD;	// UV座標
};

// 頂点出力構造体
struct VS_OUT
{
	float4 pos		: SV_POSITION;	// 頂点座標
	float4 color	: COLOR;		// 頂点カラー
	float2 uv		: TEXCOORD;	// UV座標
};

#endif	// __AKI_HLSL_H__
