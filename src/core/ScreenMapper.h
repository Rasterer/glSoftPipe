#pragma once

#include <glm/glm.hpp>

using namespace glm;

class ScreenMapper
{
public:
	ScreenMapper(): mVertexCount(0), mPosInNDC(NULL) {};
	int viewPort(int x, int y, int w, int h);
	int setupInput(size_t vertexCount, vec4 *posInNDC);
	int mapNDCToScreen();
	struct {
		unsigned int x;
		unsigned int y;
		unsigned int w;
		unsigned int h;
	} mViewPort;

	size_t mVertexCount;
	vec4 *mPosInNDC;

};
