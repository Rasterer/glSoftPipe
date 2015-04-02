#pragma once

#include <string>

class DrawEngine;
class DrawContext;

class PipeStage
{
public:
	PipeStage(const std::string &name, DrawEngine *de);
	virtual void emit(void *data) = 0;
	virtual void finalize() = 0;
	DrawContext *getDrawCtx();

	// accessors
	DrawEngine *getDrawEngine() const { return mDrawEngine; }
	PipeStage *getNextStage() const { return mNextStage; }
	const std::string & getName() const { return mName; }

	// mutators
	void setNextStage(PipeStage *stage) { mNextStage = stage; }

private:
	DrawEngine * const mDrawEngine;
	PipeStage *mNextStage;
	const std::string mName;
};
