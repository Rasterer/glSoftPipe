#pragma once

#include <glm/glm.hpp>
#include "Shader.h"
#include "VertexArrayObject.h"
#include "utils.h"

using namespace glm;

#define POSITION_INDEX 0

struct vertex
{
	int flag;
	float reg[];
};

struct viewport_t {
	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
};

struct PrimitiveContext
{
	void *vertexAttri[MAX_VERTEX_ATTRIBS];
	size_t elementSize[MAX_VERTEX_ATTRIBS];
	size_t vertexCount;
	size_t attriNum;
	int *indexBuffer;
	size_t indexBufferSize;
	size_t sizePerVertex;

	// Intermidiate index buffer
	int *mOutIB;
	size_t mOutIBSize;
	size_t mOutVBSize;

	void calVertexSize();
	vertex *poolAlloc(size_t n);
	void poolFree(vertex *pVert);
	vertex *nextVertex(vertex *pVert);
	void fetchVertex(vertex &vert, int idx);

	// In clipping space, we can use linear interpolation
	void lerp(vertex &vert, int idx1, int idx2, float t);

	void writeBackVertex(vertex *pVert, int *dstIdx, int n, int writePos);

};

class PrimitiveProcessor
{
public:
	PrimitiveProcessor();
	void attachContext(PrimitiveContext *ctx);
	void detachContext();
	void setupViewport(unsigned x, unsigned y, unsigned w, unsigned h);
	void run();

	typedef enum
	{
		CCW = 0,
		CW = 1
	} orient_t;

	typedef enum {
		BACK = 0x1,
		FRONT = 0x2,
		FRONT_AND_BACK = 0x3
	} face_t;

private:
	void primitiveAssembly();
	void clip();
	void cull();
	void perspectiveDivide();
	void viewportTransform();

	PrimitiveContext *mCtx;

// States managed
	bool mCullEnabled;
	face_t mCullFace;
	orient_t mFrontFaceOrient;

	viewport_t mViewport;

	vec4 mClipPlanes[6];
};
