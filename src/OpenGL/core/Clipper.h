#pragma once

#include <glm/glm.hpp>

#include "glsp_defs.h"
#include "PipeStage.h"
#include "DataFlow.h"

NS_OPEN_GLSP_OGL()

class Batch;
class vertex_data;

class Clipper: public PipeStage
{
public:
	// TODO: Add support for user defined clip planes
	enum PlaneType {
		/* Canonical view frustum clip planes:
		 * -w <= x <= w
		 * -w <= y <= w
		 * -w <= z <= w
		 * w > 0 (handle corner case <0, 0, 0, 0>)
		 */
		PLANE_NEAR = 0,
		PLANE_FAR,
		PLANE_LEFT,
		PLANE_RIGHT,
		PLANE_BOTTOM,
		PLANE_TOP,
		PLANE_ZEROW,

		/* Guard-band clip planes */
		PLANE_GB_LEFT,
		PLANE_GB_RIGHT,
		PLANE_GB_BOTTOM,
		PLANE_GB_TOP,
		MAX_PLANES,
	};

	Clipper();
	virtual ~Clipper() { }

	virtual void emit(void *data);
	virtual void finalize();
private:
	static void onClipping(Batch *bat);
	static void ClipAgainstFrustum(Primitive &prim, int outcodes_union, Primlist &out);
	static void ComputeOutcodesFrustum(const Primitive &prim, int outcodes[]);
	static void vertexLerp(vsOutput &new_vert,
			  vsOutput &vert1,
			  vsOutput &vert2,
			  float t);

private:
	static const glm::vec4 sPlanes[MAX_PLANES];
};

NS_CLOSE_GLSP_OGL()
