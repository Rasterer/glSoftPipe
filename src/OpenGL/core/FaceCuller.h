#pragma once

#include "PipeStage.h"


namespace glsp {

class Batch;

class FaceCuller: public PipeStage
{
public:
	enum orient_t
	{
		CCW = 0,
		CW = 1
	};

	enum face_t
	{
		BACK = 0x1,
		FRONT = 0x2,
		FRONT_AND_BACK = 0x3
	};

	FaceCuller();
	virtual ~FaceCuller() { }

	virtual void emit(void *data);
	virtual void finalize();

private:
	void culling(Batch *bat);

private:
	orient_t mOrient;
	face_t mCullFace;
};

} // namespace glsp
