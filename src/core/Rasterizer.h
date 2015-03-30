#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"

using namespace glm;
using namespace std;

class Rasterizer
{
public:
	Rasterizer(): mVertexCount(0), mVaryingNum(0), mIndexBuffer(NULL), mWidth(1280), mHeight(720) {}
	int setupInput(size_t vertexCount, size_t varyingNum, void *varyingPtr[], size_t *elementSize, int *indexBuffer, size_t indexBufferSize, FragmentShader *ps);
	int rasterizing();
	virtual int onRasterizing();

	size_t mVertexCount;
	size_t mVaryingNum;
	void *mIn[16];
	size_t mElementSize[16];
	int *mIndexBuffer;
	size_t mIndexBufferSize;
	
	char *mFrameBuffer;
	float *mDepthBuffer;
	int mLeft;
	int mBottom;
	int mWidth;
	int mHeight;

	FragmentShader *mPS;
};

// active edge table implementation
// TODO: tile-based implementation
class ScanlineRasterizer: public Rasterizer
{
public:
	ScanlineRasterizer(): Rasterizer()
	{
	}
	virtual int onRasterizing();

private:
	class edge;
	struct triangle
	{
		triangle(int index):
			mActiveEdge0(NULL),
			mActiveEdge1(NULL),
			mIndex(index)
		{
		}

		// Calculate the barycentric coodinates of point P in this triangle
		// [x0  x1  x2 ] -1    [xp ]
		// [y0  y1  y2 ]    *  [yp ]
		// [1.0 1.0 1.0]       [1.0]
		inline vec3 calculateBC(float xp, float yp)
		{
			return vec3(mMatrix * vec3(xp, yp, 1.0));
		}

		inline void setActiveEdge(edge *pEdge)
		{
			if(!mActiveEdge0)
			{
				mActiveEdge0 = pEdge;
			}
			else if(!mActiveEdge1)
			{
				mActiveEdge1 = pEdge;
			}
			else
			{
				cout << "How could this happen!" << endl;
				assert(0);
			}
		}

		inline void unSetActiveEdge(edge *pEdge)
		{
			if(pEdge == mActiveEdge0)
				mActiveEdge0 = NULL;
			else if(pEdge == mActiveEdge1)
				mActiveEdge1 = NULL;
			else
			{
				cout << "This edge is not active" << endl;
			}
		}

		inline edge * getAdjcentEdge(edge *pEdge)
		{
			if(pEdge == mActiveEdge0)
				return mActiveEdge1;

			if(pEdge == mActiveEdge1)
				return mActiveEdge0;

			cout << "How could this happen!" << endl;
			assert(0);
		}

		// There can be at most two edges in AET at the same time
		edge *mActiveEdge0;
		edge *mActiveEdge1;
		mat3 mMatrix; // mMatrix * (Xp, Yp, 1)T used to fast calculate the barycentric coordinates of P
		vec3 mDepths; // the depths of three vertics
		vec3 mW; // the W coodinates of three vertics
		int mIndex; // used to refer to the vertex attributes
	};

	struct edge
	{
		edge(triangle *pParent):
			mParent(pParent),
			bActive(false) 
		{
		}
		float x;
		float dx;
		int ymax;
		triangle *mParent;
		bool bActive;
	};

	struct span
	{
		span(float x1, float x2, triangle *pParent): xleft(x1), xright(x2), mParent(pParent) {}
		float xleft;
		float xright;
		triangle *mParent;
	};

	static bool compareFunc(edge *pEdge1, edge * pEdge2);

	void createGET();
	void scanConversion();
	void activateEdgesFromGET(int y);
	void removeEdgeFromAET(int y);
	void sortAETbyX();
	// perspective-correct interpolation
	float *interpolate(vec3 &coeff, int index);
	void traversalAET(int y);
	void advanceEdgesInAET();
	void finalize();

	vector<triangle *> mPrimitives;
	// global edges table
	map<int, vector<edge *> > mGET;
	// active edges table
	vector<edge *> mAET;

	int mYmin, mYmax;

};
