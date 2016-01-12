#pragma once

#include <glm/glm.hpp>

#include "PipeStage.h"
#include "TBDR.h"

namespace glsp {

class Batch;
class vertex_data;

// TODO:
// *.4 fixed point numbers conresponds to a 8096x8096 guard band.
#define GUARDBAND_WIDTH  8096
#define GUARDBAND_HEIGHT 8096

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

	static void ComputeGuardband(float width, float height);

private:
	static void onClipping(Batch *bat);
	static void ClipAgainstGuardband(Primitive &prim, int outcodes_union, Primlist &out);
	static void ComputeOutcodesFrustum(const Primitive &prim, int outcodes[3]);
	static void ComputeOutcodesGuardband(const Primitive &prim, int outcodes[3]);
	static void vertexLerp(vsOutput &new_vert,
			  vsOutput &vert1,
			  vsOutput &vert2,
			  float t);

private:
	static const int kGBClipMask =  (1 << PLANE_NEAR)      |
									(1 << PLANE_FAR)       |
									(1 << PLANE_GB_LEFT)   |
									(1 << PLANE_GB_RIGHT)  |
									(1 << PLANE_GB_BOTTOM) |
									(1 << PLANE_GB_TOP);
};

} // namespace glsp
