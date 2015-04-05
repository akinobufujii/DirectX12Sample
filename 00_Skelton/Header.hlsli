#ifndef __AKI_HLSL_H__
#define __AKI_HLSL_H__

// 頂点入力構造体
struct VS_IN 
{
	float4 pos		: POSITION;	// 頂点座標
	float4 color	: COLOR;	// 頂点カラー
};

// 頂点出力構造体
struct VS_OUT
{
	float4 pos		: SV_POSITION;	// 頂点座標
	float4 color	: COLOR;		// 頂点カラー
};

#endif	// __AKI_HLSL_H__
