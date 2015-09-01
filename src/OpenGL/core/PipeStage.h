#pragma once

#include <string>

#include "common/glsp_defs.h"
#include "common/glsp_debug.h"


NS_OPEN_GLSP_OGL()

class DrawEngine;
class DrawContext;

class PipeStage
{
public:
	PipeStage() = default;
	PipeStage(const std::string &name, const DrawEngine& de);
	virtual ~PipeStage() = default;

	virtual void emit(void *data) { return; }
	virtual void finalize() { return; }

	// accessors
	PipeStage* getNextStage() const
	{
		GLSP_DPF(GLSP_DPF_LEVEL_DEBUG, "Stage %s starting...\n", mNextStage->mName.c_str());
		return mNextStage;
	}

	const std::string & getName() const { return mName; }

	// mutators
	PipeStage* setNextStage(PipeStage *stage) { mNextStage = stage; return stage; }

private:
	const DrawEngine& mDrawEngine;
	PipeStage *mNextStage;
	const std::string mName;
};

NS_CLOSE_GLSP_OGL();
