#include "template/common.h"

// internal stuff
#define EDGE_THRESHOLD_MIN 	0.0312f
#define EDGE_THRESHOLD_MAX 	0.1250f
#define QUALITY 		   	10.0f
#define	ITERATIONS			12
#define SUBPIXEL_QUALITY	0.75f

// Standard luminance function
float RGB2Luminance(float3 rgb)
{
	return sqrt(dot(rgb, (float3)(0.299, 0.587, 0.114)));
}

// Optimized luminance approximation function
float RGB2LuminanceOP(float3 rgb)
{
	return rgb.y * (0.587/0.299) + rgb.x;
}

float4 TextureOffset(read_only image2d_t image, sampler_t sampler, int2 position, int2 offset)
{
	return read_imagef(image, sampler, (int2)(position + offset));
}

__kernel void copy(write_only image2d_t output, read_only image2d_t input, read_only image2d_t input1)
{
	float2 screenSize = (float2)(1280.0f, 720.0f);

	// Define sampler
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;
	
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int2 position = (int2)(x, y);
	
	float3 rgbN = TextureOffset(input, sampler, position, (int2)( 0, -1)).xyz;
	float3 rgbW = TextureOffset(input, sampler, position, (int2)(-1,  0)).xyz;
	float3 rgbM = TextureOffset(input, sampler, position, (int2)( 0,  0)).xyz;
	float3 rgbE = TextureOffset(input, sampler, position, (int2)( 1,  0)).xyz;
	float3 rgbS = TextureOffset(input, sampler, position, (int2)( 0,  1)).xyz;
	
	float lumaN = RGB2LuminanceOP(rgbN);
	float lumaW = RGB2LuminanceOP(rgbW);
	float lumaM = RGB2LuminanceOP(rgbM);
	float lumaE = RGB2LuminanceOP(rgbE);
	float lumaS = RGB2LuminanceOP(rgbS);
	
	float rangeMin = min(lumaM, min(min(lumaN, lumaW), min(lumaS, lumaE)));
	float rangeMax = max(lumaM, max(max(lumaN, lumaW), max(lumaS, lumaE)));
	
	// Check treshhold
	float range = rangeMax - rangeMin;
	if(range < max(EDGE_THRESHOLD_MIN, rangeMax * EDGE_THRESHOLD_MAX)) {
		write_imagef(output, (int2)(x, y), (float4)(TextureOffset(input1, sampler, position, (int2)( 0,  0)).xyz, 1.0f) );
		return;
	}

	float3 rgbNW = TextureOffset(input, sampler, position, (int2)(-1, -1)).xyz;
	float3 rgbNE = TextureOffset(input, sampler, position, (int2)( 1, -1)).xyz;
	float3 rgbSW = TextureOffset(input, sampler, position, (int2)(-1,  1)).xyz;
	float3 rgbSE = TextureOffset(input, sampler, position, (int2)( 1,  1)).xyz;
	
	float lumaNW = RGB2LuminanceOP(rgbNW);
	float lumaNE = RGB2LuminanceOP(rgbNE);
	float lumaSW = RGB2LuminanceOP(rgbSW);
	float lumaSE = RGB2LuminanceOP(rgbSE);
	
	float3 rgbL = (rgbN + rgbW + rgbM + rgbE + rgbS) + (rgbNW + rgbNE + rgbSW + rgbSE) * (1.0f / 9.0f);
	
	float edgeVert = fabs((0.25 * lumaNW) + (-0.5 * lumaN) + (0.25 * lumaNE)) +
					 fabs((0.50 * lumaW ) + (-1.0 * lumaM) + (0.50 * lumaE )) +
					 fabs((0.25 * lumaSW) + (-0.5 * lumaS) + (0.25 * lumaSE));
	float edgeHorz = fabs((0.25 * lumaNW) + (-0.5 * lumaW) + (0.25 * lumaSW)) +
					 fabs((0.50 * lumaN ) + (-1.0 * lumaM) + (0.50 * lumaS )) +
					 fabs((0.25 * lumaNE) + (-0.5 * lumaE) + (0.25 * lumaSE));

	bool horzSpan = edgeHorz >= edgeVert;
	
	float luma1 = horzSpan ? lumaS : lumaW;
	float luma2 = horzSpan ? lumaN : lumaE;
	
	float gradient1 = luma1 - lumaM;
	float gradient2 = luma2 - lumaM;
	
	bool  is1Steepest    = fabs(gradient1) >= fabs(gradient2);
	float gradientScaled = 0.25 * max(fabs(gradient1),fabs(gradient2));
	
	float stepLength = horzSpan ? (1.0f / screenSize.y) : (1.0f / screenSize.x);
	float lumaLocalAverage = 0.0;
	
	if(is1Steepest){
		stepLength = - stepLength;
		lumaLocalAverage = 0.5*(luma1 + lumaM);
	} 
	else 
	{
		lumaLocalAverage = 0.5*(luma2 + lumaM);
	}

	// Shift UV in the correct direction by half a pixel.
	float2 currentUv = (float2)(x / 1280.0f, y / 720.0f);
	if(horzSpan){
		currentUv.y += stepLength * 0.5;
	} else {
		currentUv.x += stepLength * 0.5;
	}
		
	
	float2 offset = horzSpan ? (float2)(1.0f / screenSize.x,0.0) : (float2)(0.0, 1.0f / screenSize.y);
	// Compute UVs to explore on each side of the edge, orthogonally. The QUALITY allows us to step faster.
	float2 uv1 = currentUv - offset;
	float2 uv2 = currentUv + offset;

	// Read the lumas at both current extremities of the exploration segment, and compute the delta wrt to the local average luma.
	float lumaEnd1 = RGB2LuminanceOP(read_imagef(input, sampler, (float2)(uv1 * screenSize)).xyz);
	float lumaEnd2 = RGB2LuminanceOP(read_imagef(input, sampler, (float2)(uv2 * screenSize)).xyz);
	lumaEnd1 -= lumaLocalAverage;
	lumaEnd2 -= lumaLocalAverage;

	// If the luma deltas at the current extremities are larger than the local gradient, we have reached the side of the edge.
	bool reached1 = fabs(lumaEnd1) >= gradientScaled;
	bool reached2 = fabs(lumaEnd2) >= gradientScaled;
	bool reachedBoth = reached1 && reached2;
	
	if(!reached1)
		uv1 -= offset;
	if(!reached2)
		uv2 += offset;  
	
	if(!reachedBoth){

		for(int i = 2; i < ITERATIONS; i++){
			// If needed, read luma in 1st direction, compute delta.
			if(!reached1){
				lumaEnd1 = RGB2LuminanceOP(read_imagef(input, sampler, (float2)(uv1) * screenSize).xyz);
				lumaEnd1 = lumaEnd1 - lumaLocalAverage;
			}
			// If needed, read luma in opposite direction, compute delta.
			if(!reached2){
				lumaEnd2 = RGB2LuminanceOP(read_imagef(input, sampler, (float2)(uv2) * screenSize).xyz);
				lumaEnd2 = lumaEnd2 - lumaLocalAverage;
			}
			// If the luma deltas at the current extremities is larger than the local gradient, we have reached the side of the edge.
			reached1 = fabs(lumaEnd1) >= gradientScaled;
			reached2 = fabs(lumaEnd2) >= gradientScaled;
			reachedBoth = reached1 && reached2;
	
			// If the side is not reached, we continue to explore in this direction, with a variable quality.
			if(!reached1)
				uv1 -= offset * QUALITY;
			if(!reached2)
				uv2 += offset * QUALITY;
	
			// If both sides have been reached, stop the exploration.
			if(reachedBoth) 
				break;
		}
	}
	
	// Compute the distances to each extremity of the edge.
	float distance1 = horzSpan ? ((x / screenSize.x) - uv1.x) : ((y / screenSize.y) - uv1.y);
	float distance2 = horzSpan ? (uv2.x - (x / screenSize.x)) : (uv2.y - (y / screenSize.y));

	// In which direction is the extremity of the edge closer ?
	bool isDirection1 = distance1 < distance2;
	float distanceFinal = min(distance1, distance2);

	// Length of the edge.
	float edgeThickness = (distance1 + distance2);

	// UV offset: read in the direction of the closest side of the edge.
	float pixelOffset = - distanceFinal / edgeThickness + 0.5;
	
	
	
	// Is the luma at center smaller than the local average ?
	bool isLumaCenterSmaller = lumaM < lumaLocalAverage;

	// If the luma at center is smaller than at its neighbour, the delta luma at each end should be positive (same variation).
	// (in the direction of the closer side of the edge.)
	bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaCenterSmaller;

	// If the luma variation is incorrect, do not offset.
	float finalOffset = correctVariation ? pixelOffset : 0.0;
	
	// Sub-pixel shifting
	// Full weighted average of the luma over the 3x3 neighborhood.
	float lumaAverage = (1.0/12.0) * (2.0 * (lumaN + lumaE + lumaS + lumaW) + (lumaNE + lumaSE + lumaSW + lumaNW));
	
	// Ratio of the delta between the global average and the center luma, over the luma range in the 3x3 neighborhood.
	float subPixelOffset1 = clamp(fabs(lumaAverage - lumaM)/range,0.0f,1.0f);
	float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
	// Compute a sub-pixel offset based on this delta.
	float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;
	
	// Pick the biggest of the two offsets.
	finalOffset = max(finalOffset,subPixelOffsetFinal);
	
	// Compute the final UV coordinates.
	float2 finalUv = (float2)(x / 1280.0f, y / 720.0f);
	if(horzSpan)
		finalUv.y += finalOffset * stepLength;
	else 
		finalUv.x += finalOffset * stepLength;


	// Read the color at the new UV coordinates, and use it.
	float4 finalColor = read_imagef(input1, sampler, (float2)(finalUv * screenSize));
	write_imagef(output, (int2)(x, y), (float4)(finalColor) );
}