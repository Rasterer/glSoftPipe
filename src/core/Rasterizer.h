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

using namespace glm;
using namespace std;

class Rasterizer: public PipeStage
{
public:
	Rasterizer();
	virtual void emit(void *data);
	virtual void finalize();
	void rasterizing(Batch *bat);

protected:
	virtual int onRasterizing(Batch *bat);
	struct fs_in_out
	{
		fsInput in;
		fsOutput out;
	}
};

// active edge table implementation
// TODO: tile-based implementation
class ScanlineRasterizer: public Rasterizer
{
public:
	ScanlineRasterizer(): Rasterizer()
	{
	}

protected:
	virtual int onRasterizing(Batch *bat);
	virtual void finalize();

private:
	class edge;

	class triangle
	{
	public:
		triangle(const Primitive *prim):
			mActiveEdge0(NULL),
			mActiveEdge1(NULL),
			mPrim(prim)
		{
		}

		vec3 calculateBC(float xp, float yp);
		void setActiveEdge(edge *pEdge);
		void unsetActiveEdge(edge *pEdge);
		edge * getAdjcentEdge(edge *pEdge);

		// There can be at most two edges in AET at the same time
		edge *mActiveEdge0;
		edge *mActiveEdge1;
		const Primitive *mPrim;
		//mat3 mMatrix; // mMatrix * (Xp, Yp, 1)T used to fast calculate the barycentric coordinates of P
		//vec3 mDepths; // the depths of three vertics
		//vec3 mW; // the W coodinates of three vertics
		//int mIndex; // used to refer to the vertex attributes
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

private:
	typedef std::unordered_map<int, vector<edge *> > GlobalEdgeTable;
	typedef std::list<edge *> ActiveEdgeTable;

	struct SRHelper
	{
		int ymin;
		int ymax;
		std::vector<triangle *> mTri;

		// global edges table
		GlobalEdgeTable mGET;

		// active edges table
		ActiveEdgeTable mAET;

		// used in following stages, e.g. fragment shader, merge.
		Shader::
		fsInput in;
		fsInput out;
	};

	static bool compareFunc(edge *pEdge1, edge * pEdge2);

	SRHelper * createGET(Batch *bat);
	void scanConversion(SRHelper *hlp, Batch *bat);
	void activateEdgesFromGET(SRHelper *hlp, int y);
	void removeEdgeFromAET(SRHelper *hlp, int y);
	void sortAETbyX();

	// perspective-correct interpolation
	void interpolate(vec3 &coeff, Primitive& prim, fsInput& result);
	void traversalAET(SRHelper *hlp, Batch *bat, int y);
	void advanceEdgesInAET();

	void finalize(SRHelper *hlp);
};
