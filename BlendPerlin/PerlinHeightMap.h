#pragma once
#include "d3dUtil.h"
#include "vertex.h"

class PerlinHeightMap
{
	inline float DotGradient(int iG, int jG, int iH, int jH) {
		float gx = gradients[iG * gWidth + jG].x;
		float gy = gradients[iG * gWidth + jG].y;

		float dx = iH - iG * factor;
		float dy = jH - jG * factor;

		return gx * dx + gy * dy;
	}

	inline float tx(float t) {
		return 6 * t*t*t*t*t - 15 * t*t*t*t + 10 * t*t*t;
	}

	inline float tt(float t) {
		return 3 * t*t - 2 * t*t*t;
	}

	inline float Lerp(float a, float b, float t) {
		float pt = tt(t);
		return a * (1.0f - pt) + b * pt;
	}

	inline float Access(float *h, int i, int j, int width) {
		return h[i*width + j];
	}

public:
	XMFLOAT2 *gradients;
	int gWidth;
	int gHeight;

	float *height;
	int mapHeight;
	int mapWidth;

	vertex *vData;
	UINT *iData;
	int M; //index count

	int factor; // equals hWidth / hMap

	PerlinHeightMap();
	PerlinHeightMap(int gh, int gw, int enlargeFactor);
	~PerlinHeightMap();

	void Refresh(int gh, int gw, int enlargeFactor);
	void Refresh();

	void GenerateGradients(int gh, int gw);
	void GenerateGradientsInternal();

	void GenerateHeight(int enlargeFactor);
	void GenerateHeightInternal();

	void LerpWith(const PerlinHeightMap &anotherA, const PerlinHeightMap &anotherB, const float t);
};

