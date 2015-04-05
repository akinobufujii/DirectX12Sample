#include "Header.hlsli"

VS_OUT main(VS_IN vsin)
{
	VS_OUT output;

	output.pos = vsin.pos;
	output.color = vsin.color;

	return output;
}
