#pragma once

#include <glm/glm.hpp>
#include "PipeStage.h"
#include "DataFlow.h"

NS_OPEN_GLSP_OGL()

class Batch;
class vertex_data;

class Clipper: public PipeStage
{
public:
	Clipper();
	virtual ~Clipper() { }

	virtual void emit(void *data);
	virtual void finalize();
	void vertexLerp(vsOutput &new_vert,
			  vsOutput &vert1,
			  vsOutput &vert2,
			  float t);

private:
	void clipping(Batch *bat);

private:
	// CVV 6 planes
	// TODO: Add support for used defined clip planes
	glm::vec4 mPlanes[6];
};

NS_CLOSE_GLSP_OGL()
