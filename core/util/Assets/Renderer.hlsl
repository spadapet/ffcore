#define NO_TEXTURE_ID 0xFFFFFFFF

static const float PI_F = 3.1415926535897932384626433832795f;
static const float PI2_F = PI_F * 2;

struct LineGeometry
{
	float2 pos0 : POSITION0;
	float2 pos1 : POSITION1;
	float2 pos2 : POSITION2;
	float2 pos3 : POSITION3;
	float4 color0 : COLOR0;
	float4 color1 : COLOR1;
	float thick0 : THICK0;
	float thick1 : THICK1;
	float depth : DEPTH;
	uint world : MATRIX;
};

struct CircleGeometry
{
	float3 pos : POSITION;
	float4 insideColor : COLOR0;
	float4 outsideColor : COLOR1;
	float radius : RADIUS;
	float thickness : THICK;
	uint world : MATRIX;
};

struct TriangleGeometry
{
	float2 pos0 : POSITION0;
	float2 pos1 : POSITION1;
	float2 pos2 : POSITION2;
	float4 color0 : COLOR0;
	float4 color1 : COLOR1;
	float4 color2 : COLOR2;
	float depth : DEPTH;
	uint world : MATRIX;
};

struct SpriteGeometry
{
	float4 rect : RECT;
	float4 uvrect : TEXCOORD;
	float4 color : COLOR;
	float2 scale : SCALE;
	float3 pos : POSITION;
	float rotate : ROTATE;
	uint tex : TEXINDEX;
	uint world : MATRIX;
};

struct MultiSpriteGeometry
{
	float4 rect : RECT;
	float4 uvrect0 : TEXCOORD0;
	float4 uvrect1 : TEXCOORD1;
	float4 uvrect2 : TEXCOORD2;
	float4 uvrect3 : TEXCOORD3;
	float4 color0 : COLOR0;
	float4 color1 : COLOR1;
	float4 color2 : COLOR2;
	float4 color3 : COLOR3;
	float2 scale : SCALE;
	float3 pos : POSITION;
	float rotate : ROTATE;
	uint4 tex : TEXINDEX;
	uint world : MATRIX;
};

struct ColorPixel
{
	float4 pos : SV_POSITION0;
	float4 color : COLOR0;
};

struct SpritePixel
{
	float4 pos : SV_POSITION0;
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
	uint tex : TEXINDEX0;
};

struct MultiSpritePixel
{
	float4 pos : SV_POSITION0;
	float4 color0 : COLOR0;
	float4 color1 : COLOR1;
	float4 color2 : COLOR2;
	float4 color3 : COLOR3;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
	float2 uv2 : TEXCOORD2;
	float2 uv3 : TEXCOORD3;
	uint4 tex0 : TEXINDEX0;
};

cbuffer GeometryShaderConstants0 : register(b0)
{
	matrix _projection;
	float2 _viewSize;
	float2 _viewScale;
	float _zoffset;
};

cbuffer GeometryShaderConstants1 : register(b1)
{
	matrix _model[1024];
};

Texture2D _textures[32] : register(t0);
SamplerState _sampler : register(s0);

LineGeometry LineVS(LineGeometry input)
{
	return input;
}

CircleGeometry CircleVS(CircleGeometry input)
{
	return input;
}

TriangleGeometry TriangleVS(TriangleGeometry input)
{
	return input;
}

SpriteGeometry SpriteVS(SpriteGeometry input)
{
	return input;
}

MultiSpriteGeometry MultiSpriteVS(MultiSpriteGeometry input)
{
	return input;
}

bool MitersOnSameSideOfLine(float2 dirLine, float2 miter1, float2 miter2)
{
	float3 lineDir_3 = float3(dirLine, 0);
	float3 offset1_3 = float3(miter1, 0);
	float3 offset2_3 = float3(miter2, 0);

	float3 cross1 = cross(lineDir_3, offset1_3);
	float3 cross2 = cross(lineDir_3, offset2_3);

	return dot(cross1, cross2) > 0;
}

[maxvertexcount(4)]
void LineGS(point LineGeometry input[1], inout TriangleStream<ColorPixel> output)
{
	ColorPixel vertex;

#ifdef SCREEN_SPACE
	float thicknessScale = _viewScale.y;
	float2 aspect = float2(_viewScale.y / _viewScale.x, 1);
#else
	float thicknessScale = 1;
	float2 aspect = float2(1, 1);
#endif

	float2 pos0 = input[0].pos0;
	float2 pos1 = input[0].pos1;
	float2 pos2 = input[0].pos2;
	float2 pos3 = input[0].pos3;

	float lineValid = any(pos1 != pos2);
	float prevValid = any(pos0 != pos1);
	float nextValid = any(pos2 != pos3);

	pos0 *= aspect;
	pos1 *= aspect;
	pos2 *= aspect;
	pos3 *= aspect;

	float2 dirLine = pos2 - pos1;
	float2 dirPrev = pos1 - pos0;
	float2 dirNext = pos3 - pos2;

	dirLine = normalize(float2(dirLine.x * lineValid + (1 - lineValid), dirLine.y * lineValid));
	dirPrev = normalize(float2(dirPrev.x * prevValid + (1 - prevValid) * dirLine.x, dirPrev.y * prevValid + (1 - prevValid) * dirLine.y));
	dirNext = normalize(float2(dirNext.x * nextValid + (1 - nextValid) * dirLine.x, dirNext.y * nextValid + (1 - nextValid) * dirLine.y));

	float2 normal = normalize(float2(-dirLine.y, dirLine.x));

	float2 tangent1 = normalize(dirPrev + dirLine);
	float2 miter1 = float2(-tangent1.y, tangent1.x);
	float miterDot1 = dot(miter1, normal);
	float miterLength1 = (input[0].thick0 * thicknessScale / 2) / (miterDot1 + (miterDot1 == 0));
	miter1 *= miterLength1;

	float2 tangent2 = normalize(dirLine + dirNext);
	float2 miter2 = float2(-tangent2.y, tangent2.x);
	float miterDot2 = dot(miter2, normal);
	float miterLength2 = (input[0].thick1 * thicknessScale / 2) / (miterDot2 + (miterDot2 == 0));
	miter2 *= miterLength2;

	float swapSide = !MitersOnSameSideOfLine(dirLine, miter1, miter2);
	miter2 *= -2 * swapSide + 1;

	float z = input[0].depth + _zoffset;
	matrix transformMatrix = mul(_model[input[0].world], _projection);
	float4 p1_up = mul(float4((pos1 + miter1) / aspect, z, 1), transformMatrix);
	float4 p1_down = mul(float4((pos1 - miter1) / aspect, z, 1), transformMatrix);
	float4 p2_up = mul(float4((pos2 + miter2) / aspect, z, 1), transformMatrix);
	float4 p2_down = mul(float4((pos2 - miter2) / aspect, z, 1), transformMatrix);

	vertex.pos = p1_down;
	vertex.color = input[0].color0;
	output.Append(vertex);

	vertex.pos = p1_up;
	vertex.color = input[0].color0;
	output.Append(vertex);

	vertex.pos = p2_down;
	vertex.color = input[0].color1;
	output.Append(vertex);

	vertex.pos = p2_up;
	output.Append(vertex);

	output.RestartStrip();
}

[maxvertexcount(128)]
void CircleGS(point CircleGeometry input[1], inout TriangleStream<ColorPixel> output)
{
	ColorPixel vertex;

#ifdef SCREEN_SPACE
	float2 thickness = input[0].thickness * _viewScale;
#else
	float2 thickness = float2(input[0].thickness, input[0].thickness);
#endif

	float z = input[0].pos.z + _zoffset;
	float2 center = input[0].pos.xy;
	float2 radius = float2(input[0].radius, input[0].radius);
	matrix transformMatrix = mul(_model[input[0].world], _projection);

	float pixelSize = length(radius / _viewScale);
	pixelSize = max(8, pixelSize);
	pixelSize = min(440, pixelSize);

	uint points = ((uint)((pixelSize - 8) / 8) + 9) & ~(uint)1;
	for (uint i = 0; i <= points; i++)
	{
		float2 sinCosAngle;
		sincos(PI2_F * i * (i != points) / points, sinCosAngle.y, sinCosAngle.x);

		float2 outsidePoint = center + sinCosAngle * radius;
		float2 insidePoint = outsidePoint - sinCosAngle * thickness;

		vertex.pos = mul(float4(insidePoint, z, 1), transformMatrix);
		vertex.color = input[0].insideColor;
		output.Append(vertex);

		vertex.pos = mul(float4(outsidePoint, z, 1), transformMatrix);
		vertex.color = input[0].outsideColor;
		output.Append(vertex);
	}

	output.RestartStrip();
}

[maxvertexcount(3)]
void TriangleGS(point TriangleGeometry input[1], inout TriangleStream<ColorPixel> output)
{
	ColorPixel vertex;

	float z = input[0].depth + _zoffset;
	float4 p0 = float4(input[0].pos0, z, 1);
	float4 p1 = float4(input[0].pos1, z, 1);
	float4 p2 = float4(input[0].pos2, z, 1);

	matrix transformMatrix = mul(_model[input[0].world], _projection);
	p0 = mul(p0, transformMatrix);
	p1 = mul(p1, transformMatrix);
	p2 = mul(p2, transformMatrix);

	vertex.pos = p0;
	vertex.color = input[0].color0;
	output.Append(vertex);

	vertex.pos = p1;
	vertex.color = input[0].color1;
	output.Append(vertex);

	vertex.pos = p2;
	vertex.color = input[0].color2;
	output.Append(vertex);

	output.RestartStrip();
}

[maxvertexcount(4)]
void SpriteGS(point SpriteGeometry input[1], inout TriangleStream<SpritePixel> output)
{
	SpritePixel vertex;
	vertex.color = input[0].color;
	vertex.tex = input[0].tex;

	float rotateSin, rotateCos;
	sincos(input[0].rotate, rotateSin, rotateCos);

	float2x2 rotateMatrix =
	{
		rotateCos, -rotateSin,
		rotateSin, rotateCos,
	};

	float4 rect = input[0].rect * float4(input[0].scale, input[0].scale);
	float z = input[0].pos.z + _zoffset;
	float4 tl = float4(mul(rect.xy, rotateMatrix) + input[0].pos.xy, z, 1);
	float4 tr = float4(mul(rect.zy, rotateMatrix) + input[0].pos.xy, z, 1);
	float4 br = float4(mul(rect.zw, rotateMatrix) + input[0].pos.xy, z, 1);
	float4 bl = float4(mul(rect.xw, rotateMatrix) + input[0].pos.xy, z, 1);

	matrix transformMatrix = mul(_model[input[0].world], _projection);
	tl = mul(tl, transformMatrix);
	tr = mul(tr, transformMatrix);
	br = mul(br, transformMatrix);
	bl = mul(bl, transformMatrix);

	vertex.pos = bl;
	vertex.uv = input[0].uvrect.xw;
	output.Append(vertex);

	vertex.pos = tl;
	vertex.uv = input[0].uvrect.xy;
	output.Append(vertex);

	vertex.pos = br;
	vertex.uv = input[0].uvrect.zw;
	output.Append(vertex);

	vertex.pos = tr;
	vertex.uv = input[0].uvrect.zy;
	output.Append(vertex);

	output.RestartStrip();
}

[maxvertexcount(4)]
void MultiSpriteGS(point MultiSpriteGeometry input[1], inout TriangleStream<MultiSpritePixel> output)
{
	MultiSpritePixel vertex;
	vertex.color0 = input[0].color0;
	vertex.color1 = input[0].color1;
	vertex.color2 = input[0].color2;
	vertex.color3 = input[0].color3;
	vertex.tex0 = input[0].tex;

	float rotateSin, rotateCos;
	sincos(input[0].rotate, rotateSin, rotateCos);

	float2x2 rotateMatrix =
	{
		rotateCos, -rotateSin,
		rotateSin, rotateCos,
	};

	float4 rect = input[0].rect * float4(input[0].scale, input[0].scale);
	float z = input[0].pos.z + _zoffset;
	float4 tl = float4(mul(rect.xy, rotateMatrix) + input[0].pos.xy, z, 1);
	float4 tr = float4(mul(rect.zy, rotateMatrix) + input[0].pos.xy, z, 1);
	float4 br = float4(mul(rect.zw, rotateMatrix) + input[0].pos.xy, z, 1);
	float4 bl = float4(mul(rect.xw, rotateMatrix) + input[0].pos.xy, z, 1);

	matrix transformMatrix = mul(_model[input[0].world], _projection);
	tl = mul(tl, transformMatrix);
	tr = mul(tr, transformMatrix);
	br = mul(br, transformMatrix);
	bl = mul(bl, transformMatrix);

	vertex.pos = bl;
	vertex.uv0 = input[0].uvrect0.xw;
	vertex.uv1 = input[0].uvrect1.xw;
	vertex.uv2 = input[0].uvrect2.xw;
	vertex.uv3 = input[0].uvrect3.xw;
	output.Append(vertex);

	vertex.pos = tl;
	vertex.uv0 = input[0].uvrect0.xy;
	vertex.uv1 = input[0].uvrect1.xy;
	vertex.uv2 = input[0].uvrect2.xy;
	vertex.uv3 = input[0].uvrect3.xy;
	output.Append(vertex);

	vertex.pos = br;
	vertex.uv0 = input[0].uvrect0.zw;
	vertex.uv1 = input[0].uvrect1.zw;
	vertex.uv2 = input[0].uvrect2.zw;
	vertex.uv3 = input[0].uvrect3.zw;
	output.Append(vertex);

	vertex.pos = tr;
	vertex.uv0 = input[0].uvrect0.zy;
	vertex.uv1 = input[0].uvrect1.zy;
	vertex.uv2 = input[0].uvrect2.zy;
	vertex.uv3 = input[0].uvrect3.zy;
	output.Append(vertex);

	output.RestartStrip();
}

float4 ColorPS(ColorPixel input) : SV_TARGET
{
	if (input.color.w == 0)
	{
		discard;
	}

	return input.color;
}

float4 SampleSpriteTexture(float2 tex, uint ntex)
{
	switch (ntex)
	{
	case NO_TEXTURE_ID: return float4(1, 1, 1, 1);
	case 0: return _textures[0].Sample(_sampler, tex);
	case 1: return _textures[1].Sample(_sampler, tex);
	case 2: return _textures[2].Sample(_sampler, tex);
	case 3: return _textures[3].Sample(_sampler, tex);
	case 4: return _textures[4].Sample(_sampler, tex);
	case 5: return _textures[5].Sample(_sampler, tex);
	case 6: return _textures[6].Sample(_sampler, tex);
	case 7: return _textures[7].Sample(_sampler, tex);
	case 8: return _textures[8].Sample(_sampler, tex);
	case 9: return _textures[9].Sample(_sampler, tex);
	case 10: return _textures[10].Sample(_sampler, tex);
	case 11: return _textures[11].Sample(_sampler, tex);
	case 12: return _textures[12].Sample(_sampler, tex);
	case 13: return _textures[13].Sample(_sampler, tex);
	case 14: return _textures[14].Sample(_sampler, tex);
	case 15: return _textures[15].Sample(_sampler, tex);
	case 16: return _textures[16].Sample(_sampler, tex);
	case 17: return _textures[17].Sample(_sampler, tex);
	case 18: return _textures[18].Sample(_sampler, tex);
	case 19: return _textures[19].Sample(_sampler, tex);
	case 20: return _textures[20].Sample(_sampler, tex);
	case 21: return _textures[21].Sample(_sampler, tex);
	case 22: return _textures[22].Sample(_sampler, tex);
	case 23: return _textures[23].Sample(_sampler, tex);
	case 24: return _textures[24].Sample(_sampler, tex);
	case 25: return _textures[25].Sample(_sampler, tex);
	case 26: return _textures[26].Sample(_sampler, tex);
	case 27: return _textures[27].Sample(_sampler, tex);
	case 28: return _textures[28].Sample(_sampler, tex);
	case 29: return _textures[29].Sample(_sampler, tex);
	case 30: return _textures[30].Sample(_sampler, tex);
	case 31: return _textures[31].Sample(_sampler, tex);
	default: return (float4)0;
	}
}

float4 SpritePS(SpritePixel input) : SV_TARGET
{
	float4 color = input.color * SampleSpriteTexture(input.uv, input.tex);

	if (color.a == 0)
	{
		discard;
	}

	return color;
}

float4 SpriteAlphaPS(SpritePixel input) : SV_TARGET
{
	float4 color = input.color * float4(1, 1, 1, SampleSpriteTexture(input.uv, input.tex).a);

	if (color.a == 0)
	{
		discard;
	}

	return color;
}

float4 GetMultiSpriteColor(MultiSpritePixel input)
{
	float4 colors[4] =
	{
		input.color0 * SampleSpriteTexture(input.uv0, input.tex0.x),
		input.color1 * SampleSpriteTexture(input.uv1, input.tex0.y),
		input.color2 * SampleSpriteTexture(input.uv2, input.tex0.z),
		input.color3 * SampleSpriteTexture(input.uv3, input.tex0.w)
	};

	for (int i = 1; i < 4; i++)
	{
		float blank = colors[0].a == 0;
		colors[0] = (1 - blank) * lerp(colors[0], float4(colors[i].rgb, 1), colors[i].aaaa) + blank * colors[i];
	}

	return colors[0];
}

float4 MultiSpritePS(MultiSpritePixel input) : SV_TARGET
{
	float4 color = GetMultiSpriteColor(input);

	if (color.a == 0)
	{
		discard;
	}

	return color;
}
