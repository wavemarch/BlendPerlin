#include "PerlinHeightMap.h"
#include <ctime>
#pragma warning( disable : 4996)


PerlinHeightMap::PerlinHeightMap(){
	memset(this, 0, sizeof(PerlinHeightMap));
	srand(time(NULL));
}

PerlinHeightMap::PerlinHeightMap(int gh, int gw, int enlargeFactor) {
	Refresh(gh, gw, enlargeFactor);
}

void PerlinHeightMap::Refresh(int gh, int gw, int enlargeFactor) {
	GenerateGradients(gh, gw);
	GenerateHeight(enlargeFactor);
}

void PerlinHeightMap::Refresh(){
	GenerateGradientsInternal();
	GenerateHeightInternal();
}


PerlinHeightMap::~PerlinHeightMap() {
	if (gradients != NULL) {
		delete[]gradients;
		gradients = NULL;
	}

	if (height != NULL){
		delete[]height;
		height = NULL;
	}

	if (vData != NULL){
		delete[]vData;
		vData = NULL;
	}

	if (iData != NULL){
		delete[]iData;
		iData = NULL;
	}
}

void PerlinHeightMap::GenerateGradients(int gh, int gw){
	if (gradients != NULL){
		delete[]gradients;
		gradients = NULL;
	}

	gHeight = gh;
	gWidth = gw;
	gradients = new XMFLOAT2[gHeight * gWidth];

	GenerateGradientsInternal();
}

void PerlinHeightMap::GenerateGradientsInternal(){
	int index = 0;
	XMFLOAT2 *p = gradients;
	for (int i = 0; i < gHeight; ++i){
		for (int j = 0; j < gWidth; ++j){
			p->x = (rand() % 100) / 100.0f;
			p->y = sqrtf(1.0f - p->x*p->x);
			++p;
		}
	}
	p = NULL;
}

void PerlinHeightMap::GenerateHeight(int enlargeFactor) {
	if (height != NULL) {
		delete[]height;
		height = NULL;
	}
	factor = enlargeFactor;
	mapHeight = (gHeight-1) * factor;
	mapWidth = (gWidth-1) * factor;
	height = new float[mapHeight * mapWidth];

	if (vData != NULL){
		delete[]vData;
		vData = NULL;
	}
	int N = mapHeight * mapWidth;
	vData = new vertex[N];

	if (iData != NULL){
		delete[]iData;
		iData = NULL;
	}
	M = (mapHeight - 1) * (mapWidth - 1) * 2 * 3;
	iData = new UINT[M];

	GenerateHeightInternal();
}

void PerlinHeightMap::GenerateHeightInternal(){
	float *p = height;
	for (int i = 0; i < mapHeight; ++i) {
		for (int j = 0; j < mapWidth; ++j) {
			int iG = i / factor;
			int jG = j / factor;

			int IG = iG + 1;
			int JG = jG + 1;

			int leftBound = jG * factor;
			int rightBound = JG * factor;

			int upBound = iG * factor;
			int lowerBound = IG*factor;

			float leftUp = DotGradient(iG, jG, i, j);
			float rightUp = DotGradient(iG, JG, i, j);
			float tL = ((float)j - jG * factor) / factor;
			float Up = Lerp(leftUp, rightUp, tL);

			float leftLow = DotGradient(IG, jG, i, j);
			float rightLow = DotGradient(IG, JG, i, j);
			float Low = Lerp(leftLow, rightLow, tL);

			float tUp = ((float)(i - iG*factor)) / factor;

			*p = Lerp(Up, Low, tUp);
			++p;
		}
	}
	p = NULL;



	vertex *pV = vData;
	float *pH = height;

	float xfactor = 2.0f;
	float yfactor = 2.0f;
	for (int i = 0; i < mapHeight; ++i) {
		for (int j = 0; j < mapWidth; ++j) {
			pV->pos.x = j * xfactor;
			pV->pos.z = i * yfactor;
			pV->pos.y = *pH * 5 - 200;

			++pV;
			++pH;
		}
	}

	pV = vData;
	for (int i = 0; i < mapHeight; ++i) {
		for (int j = 0; j < mapWidth; ++j){
			float h = Access(height, i, j, mapWidth);

			float leftH = j>0 ? Access(height, i, j - 1, mapWidth) : 0;
			float rightH = j + 1 < mapWidth ? Access(height, i, j + 1, mapWidth) : 0;

			float upH = i > 0 ? Access(height, i - 1, j, mapWidth) : 0;
			float downH = i + 1 < mapHeight ? Access(height, i + 1, j, mapWidth) : 0;

			float gx = (rightH - leftH) / (2.0f * xfactor);
			float gz = (downH - upH) / (2.0f * yfactor);

			XMFLOAT3 norm(-gx, 1.0f, -gz);
			XMVECTOR xnorm = XMLoadFloat3(&norm);
			xnorm = XMVector3Normalize(xnorm);
			XMStoreFloat3(&norm, xnorm); // need to output a log to see it

			pV->normal.x = norm.x;
			pV->normal.y = norm.y;
			pV->normal.z = norm.z;

			++pV;
		}
	}

	pV = vData;
	int uTileTimes = 4;
	int vTileTimes = 4;

	for (int i = 0; i < mapHeight; ++i){
		for (int j = 0; j < mapWidth; ++j){
			pV->tex.x = ((float)j / mapWidth);
			pV->tex.y = ((float)i / mapHeight);
			++pV;
		}
	}

	pV = NULL;
	pH = NULL;


	UINT *pI = iData;

	for (int i = 0; i + 1 < mapHeight; ++i) {
		for (int j = 0; j + 1 < mapWidth; ++j) {
			int I = i + 1;
			int J = j + 1;

			*(pI++) = i * mapWidth + j;
			*(pI++) = I * mapWidth + J;
			*(pI++) = i * mapWidth + J;

			*(pI++) = I * mapWidth + J;
			*(pI++) = i * mapWidth + j;
			*(pI++) = I * mapWidth + j;
		}
	}

	int tt = pI - iData;
	assert(M == tt);
	pI = NULL;
}

void PerlinHeightMap::LerpWith(const PerlinHeightMap &anotherA, const PerlinHeightMap &anotherB, const float t) {
	XMFLOAT2 *p = gradients;
	XMFLOAT2 *q = anotherA.gradients;
	XMFLOAT2 *w = anotherB.gradients;
	for (int i = 0; i < gHeight; ++i) {
		for (int j = 0; j < gWidth; ++j){
			XMVECTOR a = XMLoadFloat2(q);
			XMVECTOR b = XMLoadFloat2(w);
			XMVECTOR c = XMVectorLerp(a, b, t);
			XMStoreFloat2(p, c);

			++p;
			++q;
			++w;
		}
	}

	GenerateHeightInternal();
}
