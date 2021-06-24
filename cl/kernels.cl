#include "template/common.h"

// internal stuff
#define dot3(A,B)	A.x*B.x+A.y*B.y+A.z*B.z		// used in case A or B is stored in a vec4
#define OFFS_X		((bits >> 5) & 1)			// extract grid plane offset over x (0 or 1)
#define OFFS_Y		((bits >> 13) & 1)			// extract grid plane offset over y (0 or 1)
#define OFFS_Z		(bits >> 21)				// extract grid plane offset over z (0 or 1)
#define DIR_X		((bits & 3) - 1)			// ray dir over x (-1 or 1)
#define DIR_Y		(((bits >> 8) & 3) - 1)		// ray dir over y (-1 or 1)
#define DIR_Z		(((bits >> 16) & 3) - 1)	// ray dir over z (-1 or 1)
#define EPS			1e-8
#define BMSK		(BRICKDIM - 1)
#define BDIM2		(BRICKDIM * BRICKDIM)
#define BPMX		(MAPWIDTH - BRICKDIM)
#define BPMY		(MAPHEIGHT - BRICKDIM)
#define BPMZ		(MAPDEPTH - BRICKDIM)
#define TOPMASK3	(((1023 - BMSK) << 20) + ((1023 - BMSK) << 10) + (1023 - BMSK))

// fix ray directions that are too close to 0
float4 FixZeroDeltas( float4 V )
{
	if (fabs( V.x ) < EPS) V.x = V.x < 0 ? -EPS : EPS;
	if (fabs( V.y ) < EPS) V.y = V.y < 0 ? -EPS : EPS;
	if (fabs( V.z ) < EPS) V.z = V.z < 0 ? -EPS : EPS;
	return V;
}

// mighty two-level grid traversal
uint TraceRay( float4 A, const float4 B, float* dist, float3* N, float4* AC, __read_only image3d_t grid, __global const unsigned char* brick0, __global const unsigned char* brick1, int steps )
{
	__global unsigned char* brick[2] = { brick0, brick1 };

	const float4 V = FixZeroDeltas( B ), rV = (float4)(1.0 / V.x, 1.0 / V.y, 1.0 / V.z, 1);
	const bool originOutsideGrid = A.x < 0 || A.y < 0 || A.z < 0 || A.x > MAPWIDTH || A.y > MAPHEIGHT || A.z > MAPDEPTH;
	if (steps == 999999 && originOutsideGrid)
	{
		// use slab test to clip ray origin against scene AABB
		const float tx1 = -A.x * rV.x, tx2 = (MAPWIDTH - A.x) * rV.x;
		float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );
		const float ty1 = -A.y * rV.y, ty2 = (MAPHEIGHT - A.y) * rV.y;
		tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );
		const float tz1 = -A.z * rV.z, tz2 = (MAPDEPTH - A.z) * rV.z;
		tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );
		if (tmax < tmin || tmax <= 0) return 0; /* ray misses scene */ else A += tmin * V; // new ray entry point
	}
	uint4 pos = (uint4)(clamp( (int)A.x, 0, MAPWIDTH - 1 ), clamp( (int)A.y, 0, MAPHEIGHT - 1 ), clamp( (int)A.z, 0, MAPDEPTH - 1 ), 0);
	const int bits = select( 4, 34, V.x > 0 ) + select( 3072, 10752, V.y > 0 ) + select( 1310720, 3276800, V.z > 0 ); // magic
	float tmx = ((pos.x & BPMX) + ((bits >> (5 - BDIMLOG2)) & (1 << BDIMLOG2)) - A.x) * rV.x;
	float tmy = ((pos.y & BPMY) + ((bits >> (13 - BDIMLOG2)) & (1 << BDIMLOG2)) - A.y) * rV.y;
	float tmz = ((pos.z & BPMZ) + ((bits >> (21 - BDIMLOG2)) & (1 << BDIMLOG2)) - A.z) * rV.z, t = 0, dt = 0.0;
	const float tdx = DIR_X * rV.x, tdy = DIR_Y * rV.y, tdz = DIR_Z * rV.z;
	uint last = 0;
	while (true)
	{	
		// check main grid
		const uint o = read_imageui( grid, (int4)(pos.x / BRICKDIM, pos.z / BRICKDIM, pos.y / BRICKDIM, 0) ).x;
		if (o != 0) if ((o & 1) == 0) /* solid */ // NOT YET IMPLEMENTED???
		{
			*dist = t, * N = -(float3)((last == 0) * DIR_X, (last == 1) * DIR_Y, (last == 2) * DIR_Z);
			return o >> 1;
		}
		else // brick
		{
			const float4 I = A + V * t;
			uint p = (clamp( (uint)I.x, pos.x & BPMX, (pos.x & BPMX) + BMSK ) << 20) +
					 (clamp( (uint)I.y, pos.y & BPMY, (pos.y & BPMY) + BMSK ) << 10) +
					  clamp( (uint)I.z, pos.z & BPMZ, (pos.z & BPMZ) + BMSK );
			const uint pn = p & TOPMASK3;
			float dmx = (float)((p >> 20) + OFFS_X - A.x) * rV.x;
			float dmy = (float)(((p >> 10) & 1023) + OFFS_Y - A.y) * rV.y;
			float dmz = (float)((p & 1023) + OFFS_Z - A.z) * rV.z, d = t;
			do
			{	
				const int modIdx = ((o >> 1) % BRICKS_PER_BUFFER);
				const int firstId = ((o >> 1) / BRICKS_PER_BUFFER);
			
				dt = min( dmx, min( dmy, dmz ) ) - d;
				const uint idx = (modIdx) * BRICKSIZE + ((p >> 20) & BMSK) + ((p >> 10) & BMSK) * BRICKDIM + (p & BMSK) * BDIM2;
				
				const unsigned int color = brick[firstId][idx * 2];
				const unsigned int mater = brick[firstId][idx * 2 + 1];
				
				if (color != 0U)
				{
					float alpha = (mater & 7) * (1.0f/ 7.0f);
					float3 col  = INVPI * (float3)((color >> 5) * (1.0f / 7.0f), ((color >> 2) & 7) * (1.0f / 7.0f), (color & 3) * (1.0f / 3.0f));
					*dist = d, * N = -(float3)((last == 0) * DIR_X, (last == 1) * DIR_Y, (last == 2) * DIR_Z);
					if (alpha > 0.99f)
					{
						if (length((*AC).xyz) < 0.01f)
							(*AC).xyz = (float3)(1.0f, 1.0f, 1.0f);
						
						(*AC).xyz = ((*AC).xyz * (*AC).w) + (col * (1.0f - (*AC).w)); // Multiply (*AC).xyz * 
						(*AC).w   = 1.0f;
						return 0U;
					}
					else
					{
						float s = alpha * dt;
						//float pt = (*AC).w + (((1.0f - (*AC).w) * s) / 2.0f);
						//
						//(*AC).xyz += (1.0f - pt) * s * col;
						//(*AC).w   += (1.0f - pt) * s;
						
						(*AC).xyz += alpha * dt * col;			//(1.0f - (*AC).w) * s * col;
						(*AC).w   += alpha * dt;				//(1.0f - (*AC).w) * s;
						
						if ((*AC).w >= 1.0f)
						{
							(*AC).xyz /= (*AC).w;
							(*AC).w   = 1.0f;
							return 0U;
						}
					}
				}
				d = min( dmx, min( dmy, dmz ) );
				if (d == dmx) dmx += tdx, p += DIR_X << 20, last = 0;
				if (d == dmy) dmy += tdy, p += DIR_Y << 10, last = 1;
				if (d == dmz) dmz += tdz, p += DIR_Z, last = 2;
			} while ((p & TOPMASK3) == pn);
		}
		if (!--steps) break;
		t = min( tmx, min( tmy, tmz ) );
		if (t == tmx) tmx += tdx * BRICKDIM, pos.x += DIR_X * BRICKDIM, last = 0;
		if (t == tmy) tmy += tdy * BRICKDIM, pos.y += DIR_Y * BRICKDIM, last = 1;
		if (t == tmz) tmz += tdz * BRICKDIM, pos.z += DIR_Z * BRICKDIM, last = 2;
		if ((pos.x & (65536 - MAPWIDTH)) + (pos.y & (65536 - MAPWIDTH)) + (pos.z & (65536 - MAPWIDTH))) break;
	}
	return 0U;
}

float SphericalTheta( const float3 v )
{
	return acos( clamp( v.z, -1.f, 1.f ) );
}

float SphericalPhi( const float3 v )
{
	const float p = atan2( v.y, v.x );
	return (p < 0) ? (p + 2 * PI) : p;
}

uint WangHash( uint s ) { s = (s ^ 61) ^ (s >> 16), s *= 9, s = s ^ (s >> 4), s *= 0x27d4eb2d, s = s ^ (s >> 15); return s; }
uint RandomInt( uint* s ) { *s ^= *s << 13, * s ^= *s >> 17, * s ^= *s << 5; return *s; }
float RandomFloat( uint* s ) { return RandomInt( s ) * 2.3283064365387e-10f; }

float3 DiffuseReflectionCosWeighted( const float r0, const float r1, const float3 N )
{
	const float3 T = normalize( cross( N, fabs( N.y ) > 0.99f ? (float3)(1, 0, 0) : (float3)(0, 1, 0) ) );
	const float3 B = cross( T, N );
	const float term1 = TWOPI * r0, term2 = sqrt( 1 - r1 );
	float c, s = sincos( term1, &c );
	return (c * term2 * T) + (s * term2) * B + sqrt( r1 ) * N;
}

float blueNoiseSampler( const __global uint* blueNoise, int x, int y, int sampleIndex, int sampleDimension )
{
	// Adapated from E. Heitz. Arguments:
	// sampleIndex: 0..255
	// sampleDimension: 0..255
	x &= 127, y &= 127, sampleIndex &= 255, sampleDimension &= 255;
	// xor index based on optimized ranking
	int rankedSampleIndex = (sampleIndex ^ blueNoise[sampleDimension + (x + y * 128) * 8 + 65536 * 3]) & 255;
	// fetch value in sequence
	int value = blueNoise[sampleDimension + rankedSampleIndex * 256];
	// if the dimension is optimized, xor sequence value based on optimized scrambling
	value ^= blueNoise[(sampleDimension & 7) + (x + y * 128) * 8 + 65536];
	// convert to float and return
	float retVal = (0.5f + value) * (1.0f / 256.0f) /* + noiseShift (see LH2) */;
	if (retVal >= 1) retVal -= 1;
	return retVal;
}

// tc ∈ [-1,1]² | fov ∈ [0, π) | d ∈ [0,1] -  via https://www.shadertoy.com/view/tt3BRS
float3 PaniniProjection( float2 tc, const float fov, const float d )
{
	const float d2 = d * d;
	{
		const float fo = PI * 0.5f - fov * 0.5f;
		const float f = cos( fo ) / sin( fo ) * 2.0f;
		const float f2 = f * f;
		const float b = (native_sqrt( max( 0.f, (d + d2) * (d + d2) * (f2 + f2 * f2) ) ) - (d * f + f)) / (d2 + d2 * f2 - 1);
		tc *= b;
	}
	const float h = tc.x, v = tc.y, h2 = h * h;
	const float k = h2 / ((d + 1) * (d + 1)), k2 = k * k;
	const float discr = max( 0.f, k2 * d2 - (k + 1) * (k * d2 - 1) );
	const float cosPhi = (-k * d + native_sqrt( discr )) / (k + 1.f);
	const float S = (d + 1) / (d + cosPhi), tanTheta = v / S;
	float sinPhi = native_sqrt( max( 0.f, 1 - cosPhi * cosPhi ) );
	if (tc.x < 0.0) sinPhi *= -1;
	const float s = native_rsqrt( 1 + tanTheta * tanTheta );
	return (float3)(sinPhi, tanTheta, cosPhi) * s;
}

float _SignNotZero(float v)
{
	return v > 0.0f ? 1.0f : -1.0f;
}

float2 _SignNotZeroTwo(float2 v)
{
	return (float2)(_SignNotZero(v.x), _SignNotZero(v.y));
}

float2 octEncode(float3 v) {
    float l1norm = fabs(v.x) + fabs(v.y) + fabs(v.z);
    float2 result = v.xy * (1.0f / l1norm);
    if (v.z < 0.0) {
        result = ((float2)(1.0f, 1.0f) - (float2)(fabs(result.y), fabs(result.x))) * _SignNotZeroTwo(result.xy);
    }
    return result;
}

float3 octDecode(float2 o) {
    float3 v = (float3)(o.x, o.y, 1.0 - fabs(o.x) - fabs(o.y));
    if (v.z < 0.0) {
        v.xy = ((float2)(1.0f, 1.0f) - (float2)(fabs(v.y), fabs(v.x))) * _SignNotZeroTwo(v.xy);
    }
    return normalize(v);
}

int3 baseGridCoord(float3 position, float3 normal) {
	int3 difference = convert_int3(position) - (int3)(GI_POSITION_X, GI_POSITION_Y, GI_POSITION_Z);
	int3 offset = (int3)(0);
	
	if (difference.x % GI_STEP_SIZE_X == 0)
		offset.x = 1;
	if (difference.y % GI_STEP_SIZE_Y == 0)
		offset.y = 1;
	if (difference.z % GI_STEP_SIZE_Z == 0)
		offset.z = 1;
	
	offset = offset & convert_int3(normal);
	
    return clamp((convert_int3(position) - (int3)(GI_POSITION_X, GI_POSITION_Y, GI_POSITION_Z)) / (int3)(GI_STEP_SIZE_X, GI_STEP_SIZE_Y, GI_STEP_SIZE_Z),
                (int3)(0), (int3)(GI_DIMENSION_X, GI_DIMENSION_Y, GI_DIMENSION_Z) - (int3)(1)) + offset;
}

float3 gridCoordToPosition(int3 c) {
    return (float3)(GI_STEP_SIZE_X, GI_STEP_SIZE_Y, GI_STEP_SIZE_Z) * convert_float3(c) + (float3)(GI_POSITION_X, GI_POSITION_Y, GI_POSITION_Z);
}

int gridCoordToProbeIndex(int3 probeCoords) {
    return probeCoords.x + probeCoords.y * GI_DIMENSION_X + probeCoords.z * GI_DIMENSION_Y * GI_DIMENSION_Y;
}

float2 textureCoordFromDirection(float3 dir, int probeIndex)
{
   	float2 normalizedOctCoord = octEncode(normalize(dir));
    float2 normalizedOctCoordZeroOne = ((normalizedOctCoord + (float2)(1.0f, 1.0f)) * 0.5f);

	 // Length of a probe side, plus one pixel on each edge for the border
    float probeWithBorderSide = (float)GI_PROBE_RESOLUTION + 2.0f;
	
	float2 octCoordNormalizedToTextureDimensions = (normalizedOctCoordZeroOne * (float)(GI_PROBE_RESOLUTION)) / (float2)((float)(GI_PROBE_TEXTURE_WIDTH), (float)(GI_PROBE_TEXTURE_HEIGHT));
	int probesPerRow = GI_PROBE_TEXTURE_WIDTH / (int)(probeWithBorderSide);
	
	float2 probeTopLeftPosition = (float2)((probeIndex % probesPerRow) * probeWithBorderSide, (probeIndex / probesPerRow) * probeWithBorderSide) + (float2)(1.0f, 1.0f);
	float2 normalizedProbeTopLeftPosition = probeTopLeftPosition / (float2)((float)(GI_PROBE_TEXTURE_WIDTH), (float)(GI_PROBE_TEXTURE_HEIGHT));
	
	return normalizedProbeTopLeftPosition + octCoordNormalizedToTextureDimensions;
}

void bufferIndexFromDirection(float3 dir, int probeIndex, int* bufferIndex, float2* uv)
{
   	float2 normalizedOctCoord = octEncode(normalize(dir));
    float2 normalizedOctCoordZeroOne = ((normalizedOctCoord + (float2)(1.0f, 1.0f)) * 0.5f);

	 // Length of a probe side, plus one pixel on each edge for the border
    int probeWithBorderSide = GI_PROBE_RESOLUTION + 2.0f;
	
	float2 octCoordNormalizedToTextureDimensions = (normalizedOctCoordZeroOne * (float)(GI_PROBE_RESOLUTION));// / (float2)((float)(GI_PROBE_TEXTURE_WIDTH), (float)(GI_PROBE_TEXTURE_HEIGHT));
	int2 octCoordBufferIndex = convert_int2(octCoordNormalizedToTextureDimensions);
	int probesPerRow = GI_PROBE_TEXTURE_WIDTH / probeWithBorderSide;
	
	int2 probeTopLeftPosition = (int2)((probeIndex % probesPerRow) * probeWithBorderSide, (probeIndex / probesPerRow) * probeWithBorderSide) + (int2)(1, 1);
	//float2 normalizedProbeTopLeftPosition = probeTopLeftPosition / (float2)((float)(GI_PROBE_TEXTURE_WIDTH), (float)(GI_PROBE_TEXTURE_HEIGHT));
	
	int2 tbufferID = probeTopLeftPosition + octCoordBufferIndex;
	
	*bufferIndex = tbufferID.y * GI_PROBE_TEXTURE_WIDTH + tbufferID.x -1;
	//return probeTopLeftPosition + octCoordNormalizedToTextureDimensions;
}

float2 sampleDepthProbeBilerp(float3 dir, int probeIndex, __global float2* depthProbes)
{
	const float2 normalizedOctCoord = octEncode(normalize(dir));	
	const float2 normalizedOctCoordZeroOne = ((normalizedOctCoord + (float2)(1.0f, 1.0f)) * 0.5f);
	
	const int probeWithBorderSide = GI_PROBE_RESOLUTION + 2.0f;
	const float2 octCoordNormalizedToTextureDimensions = (normalizedOctCoordZeroOne * (float)(GI_PROBE_RESOLUTION));
	
	// StartingPosition
	int probesPerRow = GI_PROBE_TEXTURE_WIDTH / probeWithBorderSide;
	int2 probeTopLeftPosition = (int2)((probeIndex % probesPerRow) * probeWithBorderSide, (probeIndex / probesPerRow) * probeWithBorderSide) + (int2)(1, 1);
	
	int2 bufferIDt = probeTopLeftPosition + convert_int2(octCoordNormalizedToTextureDimensions);
	int bufferID00 = bufferIDt.y * GI_PROBE_TEXTURE_WIDTH + bufferIDt.x;
	
	float2 c00 = depthProbes[bufferID00 - GI_PROBE_TEXTURE_WIDTH - 1];
	float2 c10 = depthProbes[bufferID00 - GI_PROBE_TEXTURE_WIDTH];
	float2 c01 = depthProbes[bufferID00 - 1];
	float2 c11 = depthProbes[bufferID00];
	
	float2 t = octCoordNormalizedToTextureDimensions - convert_float2(convert_int2(octCoordNormalizedToTextureDimensions));
	
	float2 a = (1.0f - t.x) * c00 + t.x * c10;
	float2 b = (1.0f - t.x) * c01 + t.x * c11;
	return ((1 - t.y) * a + t.y * b);
}

float4 sampleIrradianceProbe(float3 dir, int probeIndex, __global float4* depthProbes)
{
	const float2 normalizedOctCoord = octEncode(normalize(dir));	
	const float2 normalizedOctCoordZeroOne = ((normalizedOctCoord + (float2)(1.0f, 1.0f)) * 0.5f);
	
	const int probeWithBorderSide = GI_PROBE_RESOLUTION + 2.0f;
	const float2 octCoordNormalizedToTextureDimensions = (normalizedOctCoordZeroOne * (float)(GI_PROBE_RESOLUTION));
	
	// StartingPosition
	int probesPerRow = GI_PROBE_TEXTURE_WIDTH / probeWithBorderSide;
	int2 probeTopLeftPosition = (int2)((probeIndex % probesPerRow) * probeWithBorderSide, (probeIndex / probesPerRow) * probeWithBorderSide) + (int2)(1, 1);
	
	int2 bufferIDt = probeTopLeftPosition + convert_int2(octCoordNormalizedToTextureDimensions);
	int bufferID00 = bufferIDt.y * GI_PROBE_TEXTURE_WIDTH + bufferIDt.x;
	
	return depthProbes[bufferID00];
}

float4 sampleIrradianceProbeBilerp(float3 dir, int probeIndex, __global float4* depthProbes)
{
	const float2 normalizedOctCoord = octEncode(normalize(dir));	
	const float2 normalizedOctCoordZeroOne = ((normalizedOctCoord + (float2)(1.0f, 1.0f)) * 0.5f);
	
	const int probeWithBorderSide = GI_PROBE_RESOLUTION + 2.0f;
	const float2 octCoordNormalizedToTextureDimensions = (normalizedOctCoordZeroOne * (float)(GI_PROBE_RESOLUTION));
	
	// StartingPosition
	int probesPerRow = GI_PROBE_TEXTURE_WIDTH / probeWithBorderSide;
	int2 probeTopLeftPosition = (int2)((probeIndex % probesPerRow) * probeWithBorderSide, (probeIndex / probesPerRow) * probeWithBorderSide) + (int2)(1, 1);
	
	int2 bufferIDt = probeTopLeftPosition + convert_int2(octCoordNormalizedToTextureDimensions);
	int bufferID00 = bufferIDt.y * GI_PROBE_TEXTURE_WIDTH + bufferIDt.x;
	
	float4 c00 = depthProbes[bufferID00 - GI_PROBE_TEXTURE_WIDTH - 1];
	float4 c10 = depthProbes[bufferID00 - GI_PROBE_TEXTURE_WIDTH];
	float4 c01 = depthProbes[bufferID00 - 1];
	float4 c11 = depthProbes[bufferID00];
	
	float2 t = octCoordNormalizedToTextureDimensions - convert_float2(convert_int2(octCoordNormalizedToTextureDimensions));
	
	float4 a = (1.0f - t.x) * c00 + t.x * c10;
	float4 b = (1.0f - t.x) * c01 + t.x * c11;
	return ((1 - t.y) * a + t.y * b);
}

float3 probeIndexToPosition(int _index)
{
	// Calculate gridCoord
	float3 gridCoord;
	gridCoord.x =  _index %  GI_DIMENSION_X;
	gridCoord.y = (_index % (GI_DIMENSION_X * GI_DIMENSION_Y)) / GI_DIMENSION_X;
	gridCoord.z =  _index / (GI_DIMENSION_X * GI_DIMENSION_Y);
	
	return (convert_float3(gridCoord) * (float3)(GI_STEP_SIZE_X, GI_STEP_SIZE_Y, GI_STEP_SIZE_Z)) + (float3)(GI_POSITION_X, GI_POSITION_Y, GI_POSITION_Z);
}

float3 probeIndexToPosition1(int _index)
{
	// Calculate gridCoord
	float3 gridCoord;
	gridCoord.x =  _index %  GI_DIMENSION_X;
	gridCoord.y = (_index % (GI_DIMENSION_X * GI_DIMENSION_Y)) / GI_DIMENSION_X;
	gridCoord.z =  _index / (GI_DIMENSION_X * GI_DIMENSION_Y);
	
	return convert_float3(gridCoord) / (float3)(3.0f);
}

bool insideCage(float3 pos)
{
	if (pos.x <= GI_POSITION_X || pos.y <= GI_POSITION_Y || pos.z <= GI_POSITION_Z)
		return false;
	if (pos.x >= GI_POSITION_X + (GI_STEP_SIZE_X * GI_DIMENSION_X) || pos.y >= GI_POSITION_Y + (GI_STEP_SIZE_Y * GI_DIMENSION_Y) || pos.z >= GI_POSITION_Z + (GI_STEP_SIZE_Z * GI_DIMENSION_Z))
		return false;
	
	return true;
}

__kernel void render( write_only image2d_t outimg, __constant struct RenderParams* params, __read_only image3d_t grid, __global unsigned char* brick, __global unsigned char* brickMaterial, 
__global float4* sky, __global const uint* blueNoise, __global float4* irradianceProbes, __global float2* depthProbes, __global float4* pathTraceResult)
{
	// produce primary ray for pixel
	const int column = get_global_id( 0 );
	const int line = get_global_id( 1 );
	const float2 uv = (float2)((float)column * params->oneOverRes.x, (float)line * params->oneOverRes.y);
#if PANINI
	const float3 V = PaniniProjection( (float2)(uv.x * 2 - 1, (uv.y * 2 - 1) * ((float)SCRHEIGHT / SCRWIDTH)), PI / 5, 0.15f );
	// multiply by improvised camera matrix
	const float3 D = V.z * normalize( (params->p1 + params->p2) * 0.5f - params->E ) +
		V.x * normalize( params->p1 - params->p0 ) +
		V.y * normalize( params->p2 - params->p0 );
#else
	const float3 P = params->p0 + (params->p1 - params->p0) * uv.x + (params->p2 - params->p0) * uv.y;
	const float3 D = normalize( P - params->E );
#endif

	// trace primary ray
	float dist;
	float3 N;
	float4 A = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
	const uint voxel = TraceRay( (float4)(params->E, 1), (float4)(D, 1), &dist, &N, &A, grid, brick, brickMaterial, 999999 /* no cap needed */ );
	const float3 CO = INVPI * (float3)((voxel >> 5) * (1.0f / 7.0f), ((voxel >> 2) & 7) * (1.0f / 7.0f), (voxel & 3) * (1.0f / 3.0f));
	// visualize result
	float3 pixel;
	
#if GI_PATH_TRACE_INDIRECT_ON
	if (params->frame == 0)
		irradianceProbes[line * SCRWIDTH + column] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);

	float4 result = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
	
	// Generate random direction
	int seed = WangHash(column * 171 + line * 1773 + params->R0); // column * 171 + line * 1773 + params->R0
	const float r0 = RandomFloat(&seed);
	const float r1 = RandomFloat(&seed);
	
	const float theta = r0 * TWOPI;
	const float phi   = r1 * TWOPI;
		
	// Calculate worldPos
	float3 worldPos = params->E + (D * dist);
	const float3 localDirection = DiffuseReflectionCosWeighted( r0, r1, N );
	//const float3 localDirection = normalize((float3)(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta))) * N;
	
	// Trace rays
	float dist1;
	float3 N1 = (float3)(0.0f);
	float4 A1 = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
	const uint voxel1 = TraceRay( (float4)(worldPos, 1), (float4)(localDirection, 1), &dist1, &N1, &A1, grid, brick, brickMaterial, 999999 /* no cap needed */ );
	
	float3 color1 = (float3)(0.0f);
	if (dist1 < 100 && length(A1.xyz) > 0.01f)
	{
		float weight = dot(N, localDirection);		
		color1 = (pathTraceResult[line * SCRWIDTH + column].xyz * pathTraceResult[line * SCRWIDTH + column].w + A1.xyz * weight) / (pathTraceResult[line * SCRWIDTH + column].w + 1.0f) ;
		pathTraceResult[line * SCRWIDTH + column].w += 1.0f;
		
		pathTraceResult[line * 1280 + column].xyz = color1;
	}
	
	if (length(color1) < 0.01f)
		color1 = pathTraceResult[line * 1280 + column].xyz;
	
	write_imagef( outimg, (int2)(column, line), (float4)(color1.xyz, 1.0f) );
	return;
	
#elseif GI_PROBES_ON
	//// Calculate worldPos
	float3 worldPos = params->E + (D * dist);
	float3 viewVec  = normalize(params->E - worldPos);
	
	if (!insideCage(worldPos))
	{
		write_imagef( outimg, (int2)(column, line), (float4)(1.0f, 1.0f, 1.0f, 1.0f) );
		return;
	}
	
	// Calculate gridCoords
	int3   baseGrid = baseGridCoord(worldPos, N);
	int    baseProbeID  = gridCoordToProbeIndex(baseGrid);
	float3 baseProbePos = gridCoordToPosition(baseGrid);
	
	// Error check
	//if (baseGrid.x <= 0 || baseGrid.y <= 0 || baseGrid.z <= 0)
	//{
	//	write_imagef( outimg, (int2)(column, line), (float4)(0.0f, 0.0f, 0.0f, 1.0f) );
	//	return;
	//}
	
	float energyPreservation = 0.95f;
	
	int   GI_DEBUG_VALUE0 = 7;
	float GI_DEBUG_RESULT = 0.0f;
	
	// Probe weight
	float3 alpha = clamp((worldPos - baseProbePos) / (float3)(GI_STEP_SIZE_X, GI_STEP_SIZE_Y, GI_STEP_SIZE_Z), (float3)(0.0f), (float3)(1.0f));
	float4 result = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
	for (int i = 0; i < 8; ++i)
	{
		// Probe offset
		int3 offset = (int3)(i, i>>1, i>>2) & (int3)(1);
		
		int3 probeGridCoord = baseGrid + offset;//clamp(baseGrid + offset, (int3)(0, 0, 0), (int3)((GI_DIMENSION_X, GI_DIMENSION_Y, GI_DIMENSION_Z) - (int3)(1, 1, 1)));
        int  p = gridCoordToProbeIndex(probeGridCoord);
	
		float3 probePos = gridCoordToPosition(probeGridCoord);
		
		float3 probeToPoint = (worldPos - probePos) + normalize(N + (float3)(1.0f) * viewVec) * 0.01f;
        float3 dir = normalize(-probeToPoint);
		
		float3 trilinear;
		trilinear.x = mix(1.0f - alpha.x, alpha.x, offset.x);
		trilinear.y = mix(1.0f - alpha.y, alpha.y, offset.y);
		trilinear.z = mix(1.0f - alpha.z, alpha.z, offset.z);
		
		float weight = 1.0;
		{
			float3 trueDirectionToProbe = normalize(probePos - worldPos);
			weight *= max(0.0001f, dot(trueDirectionToProbe, N));
		}
	    {
			float2 depth = sampleDepthProbeBilerp(-dir, p, depthProbes);//depthProbes[bufferID];
			float distance = length(probeToPoint);
			
			float mean = depth.x;
            float variance = fabs((depth.x * depth.x) - depth.y);

			float t = max(distance - mean, 0.0f) * max(distance - mean, 0.0f);
            float chebyshevWeight = variance / (variance + t);
            chebyshevWeight = max(chebyshevWeight * chebyshevWeight * chebyshevWeight, 0.0f);
			
			weight *= (distance <= mean) ? 1.0f : chebyshevWeight;
	    }
		weight = max(0.000001f, weight);
	
		
		float3 probeIrradiance = sampleIrradianceProbeBilerp(N, p, irradianceProbes).xyz;
	
		const float crushThreshold = 0.2;
		if (weight < crushThreshold) 
			weight *= weight * weight * (1.0 / (crushThreshold * crushThreshold)); 

		// Trilinear weights
		weight *= trilinear.x * trilinear.y * trilinear.z;
		
	//
		//probeIrradiance = sqrt(probeIrradiance);
	
	
	//	weight = clamp(weight, 0.0f, 1.0f);
		//if(i == 7)
		{
			result.xyz += probeIrradiance * weight;//(float3)(dephts.x / 10.0f, dephts.y / 10.0f, dephts.z / 10.0f);//sample.xyz;// (trueDirectionToProbe + (float3)(1.0f)) / (float3)(2.0f);// * (float3)(0.2f, 0.2f, 0.2f);	
			result.w   += weight;
		}
	}
	

	//result.xyz /= result.w;	
	//result.xyz = result.xyz * result.xyz;
	
	//result.xyz = convert_float3(baseGrid) / 7.0f;
	result.xyz = (float3)(result.w / 4.0f);
	
	//if (voxel == 0)
	{
		// sky
		const float3 T = (float3)(D.x, D.z, D.y);
		const uint u = (uint)(5000 * SphericalPhi( T ) * INV2PI - 0.5f);
		const uint v = (uint)(2500 * SphericalTheta( T ) * INVPI - 0.5f);
		const uint idx = u + v * 5000;
		pixel = (idx < 5000 * 2500) ? sky[idx].xyz : (float3)(1);
		
		pixel = (A.xyz * A.w);// + (pixel * (1.0f - A.w));
		
		const float3 BRDF1 = (float3)(A.x * result.x + A.x, A.y * result.y + A.y, A.z * result.z + A.z);//(A.xyz * result.xyz) + A.xyz;

		write_imagef( outimg, (int2)(column, line), (float4)(result.xyz * A.xyz, 1.0f) );
		return;
	}
	//else
		
	#else
	{
		const float3 BRDF1 = INVPI * A.xyz;//(float3)((voxel >> 5) * (1.0f / 7.0f), ((voxel >> 2) & 7) * (1.0f / 7.0f), (voxel & 3) * (1.0f / 3.0f));
	#if GIRAYS > 0
		float3 incoming = (float3)(0, 0, 0);
		uint seed = WangHash( column * 171 + line * 1773 + params->R0 );
		const float4 I = (float4)(params->E + D * dist, 1);
		for (int i = 0; i < GIRAYS; i++)
		{
			const float r0 = blueNoiseSampler( blueNoise, column, line, i + GIRAYS * params->frame, 0 );
			const float r1 = blueNoiseSampler( blueNoise, column, line, i + GIRAYS * params->frame, 1 );
			const float4 R = (float4)(DiffuseReflectionCosWeighted( r0, r1, N ), 1);
			float3 N2;
			float4 A2;
			float dist2;
			const uint voxel2 = TraceRay( I + 0.1f * (float4)(N, 1), R, &dist2, &N2, &A2, grid, brick, brickMaterial, GRIDWIDTH / 12 /* cap on GI ray length */ );
			if (voxel2 == 0)
			{
				// sky
				const float3 T = (float3)(R.x, R.z, R.y);
				const uint u = (uint)(5000 * SphericalPhi( T ) * INV2PI - 0.5f);
				const uint v = (uint)(2500 * SphericalTheta( T ) * INVPI - 0.5f);
				const uint idx = u + v * 5000;
				incoming += 4 * ((idx < 5000 * 2500) ? sky[idx].xyz : (float3)(1));
			}
			else
			{
				float3 BRDF2 = INVPI * (float3)((voxel2 >> 5) * (1.0f / 7.0f), ((voxel2 >> 2) & 7) * (1.0f / 7.0f), (voxel2 & 3) * (1.0f / 3.0f));
				// secondary hit
				incoming += BRDF2 * 2 * (
					(N2.x * N2.x) * ((-N2.x + 1) * (float3)(NX0) + (N2.x + 1) * (float3)(NX1)) +
					(N2.y * N2.y) * ((-N2.y + 1) * (float3)(NY0) + (N2.y + 1) * (float3)(NY1)) +
					(N2.z * N2.z) * ((-N2.z + 1) * (float3)(NZ0) + (N2.z + 1) * (float3)(NZ1))
				);
			}
		}
		pixel = BRDF1 * incoming * (1.0f / GIRAYS);
	#else
		// hardcoded lights - image based lighting, no visibility test
		pixel = BRDF1 * 2 * (
			(N.x * N.x) * ((-N.x + 1) * (float3)(NX0) + (N.x + 1) * (float3)(NX1)) +
			(N.y * N.y) * ((-N.y + 1) * (float3)(NY0) + (N.y + 1) * (float3)(NY1)) +
			(N.z * N.z) * ((-N.z + 1) * (float3)(NZ0) + (N.z + 1) * (float3)(NZ1))
		);
		
		write_imagef( outimg, (int2)(column, line), (float4)(pixel.xyz, 1) );
	#endif
	}
	
	write_imagef( outimg, (int2)(column, line), (float4)(pixel.xyz, 1) );
	#endif
}

__kernel void commit( const int taskCount, __global uint* commit, __global uint* brick0, __global uint* brick1 )
{
	__global uint* brick[2] = { brick0, brick1 };

	// put bricks in place
	int task = get_global_id( 0 );
	if (task < taskCount)
	{
		int brickId = (commit[task + GRIDSIZE] % BRICKS_PER_BUFFER);
		int firstId = (commit[task + GRIDSIZE] / BRICKS_PER_BUFFER);
		
		__global uint* srcB = commit + MAXCOMMITS + GRIDSIZE + (task * 2) * BRICKSIZE / 4;
		__global uint* dstB = brick[firstId] + (brickId * 2) * BRICKSIZE / 4;
		
		for (int i = 0; i < (BRICKSIZE * 2) / 4; i++)
		{
			dstB[i] = srcB[i];
		}
	}
}

float2 TexCoordToProbeTexCoord(int2 fragCoord, int sideLength) {
    int probeWithBorderSide = sideLength + 2;

    float2 octFragCoord = (float2)((fragCoord.x) % probeWithBorderSide, (fragCoord.y) % probeWithBorderSide);
    // Add back the half pixel to get pixel center normalized coordinates
    return (octFragCoord + (float2)(0.5f, 0.5f)) * (2.0f / sideLength) - (float2)(1.0f, 1.0f);
}

int TexCoordToProbeID(int2 texelXY, int2 texDim, int sideLength) {
    int probeWithBorderSide = sideLength + 2;
    int probesPerSide = texDim.x / probeWithBorderSide;
    return (texelXY.x / probeWithBorderSide) + probesPerSide * (texelXY.y / probeWithBorderSide);
}

__kernel void traceProbes(__global float4* irradianceOutput, __global float4* normalOutput, __global float2* depthOutput, __global const uint* blueNoise, __constant struct RenderParams* params,
		__read_only image3d_t grid, __global unsigned char* brick, __global unsigned char* brickMaterial)//, write_only image2d_t debugOutput)
{	
	// Retreive variables
	const int probeID 	= get_global_id(0);
	const int rayID 	= get_global_id(1);
	const int2 position = (int2)(probeID, rayID);
	
	if (probeID >= GI_PROBE_COUNT)  return;
	if (rayID > 200) return;
	
	// Generate random direction
	int seed = WangHash(probeID * 171 + rayID * 1773 + params->R0); // column * 171 + line * 1773 + params->R0
	const float r0 = RandomFloat(&seed);
	const float r1 = RandomFloat(&seed);
	
	const float theta = r0 * TWOPI;
	const float phi   = r1 * TWOPI;
	const float3 localDirection = normalize((float3)(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta)));
	
	const float3 pos = probeIndexToPosition(probeID);
	//const float3 pos = (float3)(210, 110, 210);
	
	// Trace rays
	float dist;
	float3 N = (float3)(0.0f);
	float4 A = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
	const uint voxel = TraceRay( (float4)(pos, 1), (float4)(localDirection, 1), &dist, &N, &A, grid, brick, brickMaterial, 999999 /* no cap needed */ );
	
	// Write output
	irradianceOutput[probeID * GI_RAYS_PER_PROBE + rayID] = (float4)(A.xyz, 1.0f);
	normalOutput[probeID * GI_RAYS_PER_PROBE + rayID] = (float4)(((localDirection + (float3)(1.0f)) / (float3)(2.0f)), 1.0f);
	depthOutput[probeID * GI_RAYS_PER_PROBE + rayID] = (float2)(dist, dist * dist);
	
	//write_imagef( debugOutput, (int2)(probeID, rayID), (float4)(pos.xyz / (float3)(400.0f), 1.0f) );
	//write_imagef( debugOutput, (int2)(probeID + GI_PROBE_COUNT, rayID), (float4)(dist / 10.0f, dist / 100.0f, dist / 250.0f , 1.0f) );
	//write_imagef( debugOutput, (int2)(probeID + GI_PROBE_COUNT * 2, rayID), (float4)(localDirection , 1.0f) );
}

__kernel void updateProbes(__global float4* irradianceOutput, __global float2* depthOutput, __global float4* irradianceInput, __global float4* normalInput, 
		__global float2* depthInput)//, write_only image2d_t debugOutput)
{
	const float energyConservation = 0.05f;
	
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int2 position = (int2)(x, y);
	
	int probeID = TexCoordToProbeID((int2)(x, y), (int2)(GI_PROBE_TEXTURE_WIDTH, GI_PROBE_TEXTURE_HEIGHT), GI_PROBE_RESOLUTION);
	if (probeID >= GI_PROBE_COUNT || probeID < 0)
		return;
	
	if (x >= GI_PROBE_TEXTURE_WIDTH ) return;
	if (y >= GI_PROBE_TEXTURE_HEIGHT) return;
	
	float2 localUV = TexCoordToProbeTexCoord(position, GI_PROBE_RESOLUTION);
	float3 localDirection = octDecode(localUV);
	
	float4 irradianceResult = (float4)(0.0f);
	float3 depthResult = (float3)(0.0f);
	for (int i = 0; i < GI_RAYS_PER_PROBE; i++)
	{
		float3 normal = normalInput[(probeID * GI_RAYS_PER_PROBE) + i].xyz;
		float3 direction = normal * (float3)(2.0f, 2.0f, 2.0f) - (float3)(1.0f, 1.0f, 1.0f);
		
		float weight 	  = max(0.0f, dot(localDirection, direction));
		float depthWeight = pow(weight, 4.0f);
		
		float4 irradiance = irradianceInput[(probeID * GI_RAYS_PER_PROBE) + i];
		float2 depth = depthInput[(probeID * GI_RAYS_PER_PROBE) + i];
		
		if (depth.x > 100)
			continue;
		
		if (weight > 0.01f && length(irradiance.xyz) > 0.01f)
		{
			irradianceResult += (float4)(irradiance.xyz * weight, weight);	
		}
		
		if (depthWeight > 0.01f&& length(irradiance.xyz) > 0.01f)
		{
			depthResult += (float3)(depth.xy * depthWeight, depthWeight);
		}
	}

	if (irradianceResult.w > 0.01f)
	{
		irradianceResult.xyz /= irradianceResult.w;
		irradianceOutput[y * GI_PROBE_TEXTURE_WIDTH + x] *= (1.0f - energyConservation);
		irradianceOutput[y * GI_PROBE_TEXTURE_WIDTH + x] += (irradianceResult * energyConservation);
	}	
	
	if (depthResult.z > 0.01f)
	{
		depthResult.xy /= depthResult.z;
		depthOutput[y * GI_PROBE_TEXTURE_WIDTH + x] *= (1.0f - energyConservation);
		depthOutput[y * GI_PROBE_TEXTURE_WIDTH + x] += (energyConservation * depthResult.xy);
	}	
	
	//irradianceOutput[y * GI_PROBE_TEXTURE_WIDTH + x] *= (1.0f - energyConservation);
	//irradianceOutput[y * GI_PROBE_TEXTURE_WIDTH + x] += (irradianceResult * energyConservation);
	//irradianceOutput[y * GI_PROBE_TEXTURE_WIDTH + x] = (float4)(irradianceResult.xyz, 1.0f);
	
	//depthOutput[y * GI_PROBE_TEXTURE_WIDTH + x] *= (1.0f - energyConservation);
	//depthOutput[y * GI_PROBE_TEXTURE_WIDTH + x] += (energyConservation * depthResult.xy);
	//depthOutput[y * GI_PROBE_TEXTURE_WIDTH + x] = depthResult.xy;
	
	//float3 sample = sky[y * GI_PROBE_TEXTURE_WIDTH + x].xyz;
	//sample = (float3)(max(sample.x, 0.0f), max(sample.y, 0.0f), max(sample.z, 0.0f));
	//irradianceResult.xyz = (energyConservation * irradianceResult.xyz) + ((1.0f - energyConservation) * sample);
	
	//result.xyz = (localDirection + (float3)(1.0f, 1.0f, 1.0f)) / (float3)(2.0f, 2.0f, 2.0f);
	//result.xyz = (result.xyz + (float3)(1.0f, 1.0f, 1.0f)) / (float3)(2.0f, 2.0f, 2.0f);
	
	//write_imagef(output, (int2)(x, y), (float4)(0.0f, 1.0f, 0.0, 1.0f) );
	
	//depthResult.xy = depthOutput[y * GI_PROBE_TEXTURE_WIDTH + x];
	
	//float3 pos = probeIndexToPosition(probeID) / (float3)(100.0f);
	//write_imagef( debugOutput, (int2)(x, y), (float4)(depthResult.x / 10.0f, depthResult.x / 25.0f, depthResult.x / 100.0f, 1.0f));
	//write_imagef( debugOutput, (int2)(x, y), (float4)(irradianceResult.xyz, 1.0f));
	//write_imagef( debugOutput, (int2)(x, y), (float4)((localDirection + (float3)(1.0f)) / (float3)(2.0f), 1.0f) );
}

// Notes

// Reprojection, according to Lighthouse 2:
// float3 D = normalize( localWorldPos - prevPos );
// float3 S = prevPos + D * (prevPos.w / dot( prevE, D ));
// prevPixelPos = make_float2( dot( S, prevRight ) - prevRight.w - j0, dot( S, prevUp ) - prevUp.w - j1 );
// data:
// -----------
// PREP:
// float3 centre = 0.5f * (prevView.p2 + prevView.p3);
// float3 direction = normalize( centre - prevView.pos );
// float3 right = normalize( prevView.p2 - prevView.p1 );
// float3 up = normalize( prevView.p3 - prevView.p1 );
// float focalDistance = length( centre - prevView.pos );
// float screenSize = length( prevView.p3 - prevView.p1 );
// float lenReci = h / screenSize;
// -----------
// float4 prevPos = make_float4( prevView.pos, -(dot( prevView.pos, direction ) - dot( centre, direction )) )
// float4 prevE = make_float4( direction, 0 )
// float4 prevRight = make_float4( right * lenReci, dot( prevView.p1, right ) * lenReci )
// float4 prevUp = make_float4( up * lenReci, dot( prevView.p1, up ) * lenReci )

// Faster empty space skipping:
// bits for empty top-level grid cells didn't work.
// Try instead: https://www.kalojanov.com/data/irregular_grid.pdf
// as suggested by Guillaume Boissé.