#pragma once

#include <string>

#include "glsp_defs.h"
#include "glsp_debug.h"


NS_OPEN_GLSP_OGL()

class DrawEngine;
class DrawContext;

class PipeStage
{
public:
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

protected:
	const DrawEngine& mDrawEngine;
	PipeStage *mNextStage;
	const std::string mName;
};


// TODO:
class PipeStageChain: public PipeStage
{
public:
	PipeStageChain(const std::string &name, const DrawEngine& de);
	~PipeStageChain() = default;

	PipeStage* getFirstChild() const { return mFirstChild; }
	void setFirstChild(PipeStage *child) { mFirstChild = child; }

	PipeStage* getLastChild() const { return mLastChild; }
	void setLastChild(PipeStage *child) { mLastChild = child; }

protected:
	PipeStage *mFirstChild;
	PipeStage *mLastChild;
};


NS_CLOSE_GLSP_OGL();
