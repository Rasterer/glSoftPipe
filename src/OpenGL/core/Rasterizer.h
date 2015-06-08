#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"

NS_OPEN_GLSP_OGL()

using namespace std;

class Rasterizer: public PipeStage
{
public:
	Rasterizer();
	virtual ~Rasterizer() { }

	virtual void emit(void *data);
	virtual void finalize();
	void rasterizing(Batch *bat);

	struct fs_in_out
	{
		fsInput in;
		fsOutput out;
	};
protected:
	virtual int onRasterizing(Batch *bat);
};

// active edge table implementation
// TODO: tile-based implementation
class ScanlineRasterizer: public Rasterizer
{
public:
	ScanlineRasterizer(): Rasterizer()
	{
	}

	virtual ~ScanlineRasterizer() { }

protected:
	virtual int onRasterizing(Batch *bat);
	virtual void finalize();

private:
	class edge;
	class triangle;
	class span;
	class SRHelper;

	static bool compareFunc(edge *pEdge1, edge *pEdge2);

	SRHelper* createGET(Batch *bat);
	void scanConversion(SRHelper *hlp, Batch *bat);
	void activateEdgesFromGET(SRHelper *hlp, int y);
	void removeEdgeFromAET(SRHelper *hlp, int y);
	//void sortAETbyX();

	// perspective-correct interpolation
	void interpolate(glm::vec3 &coeff, Primitive& prim, fsInput& result);
	void traversalAET(SRHelper *hlp, Batch *bat, int y);
	void advanceEdgesInAET(SRHelper *hlp);

	void finalize(SRHelper *hlp);
};

NS_CLOSE_GLSP_OGL()
