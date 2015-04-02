#pragma once

#include <glm/glm.hpp>
#include "PipeStage.h"

class Batch;
class vertex_data;

class Clipper: public PipeStage
{
public:
	Clipper();
	virtual void emit(void *data);
	virtual void finalize();
	void clipping(Batch *bat);
	void vertexLerp(vertex_data &new_vert,
			  vertex_data &vert1,
			  vertex_data &vert2,
			  float t);

private:
	// CVV 6 planes
	// TODO: Add support for used defined clip planes
	glm::vec4 mPlanes[6];
};
