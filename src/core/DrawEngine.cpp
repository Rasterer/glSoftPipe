#include "DrawEngine.h"

GLAPI void APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	__GET_CONTEXT();

	DrawEngine *de = DrawEngine::getDrawEngine();

	DrawContext *dc = new DrawContext();

	// encapsulate the DrawContext prepare work
	dc->gc = gc;
	dc->mMode = mode;
	dc->mFirst = first;
	dc->mCount = count;
	dc->mDrawType = DrawContext::kArrayDraw;

	de->validateState(dc);

	de->emit(dc);
}

GLAPI void APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	__GET_CONTEXT();

	DrawEngine *de = DrawEngine::getDrawEngine();

	DrawContext *dc = new DrawContext();

	dc->gc = gc;
	dc->mMode = mode;
	dc->mFirst = 0;
	dc->mCount = count;
	dc->mDrawType = DrawContext::kElementDraw;
	dc->mIndexSize = (type == GL_UNSIGNED_INT)? 4: ((type == GL_UNSIGNED_SHORT)? 2: 1);
	dc->mIndices = indices;

	de->validateState(dc);

}
DrawEngine::DrawEngine():
	mCtx(NULL),
	mFirstStage(NULL)
{
}

// TODO(done): connect all the pipeline stages
// Shaders are inserted in validateState()
void DrawEngine::init()
{
	setFistStage(&mAsbl);

	mPrimAsbl.setNextStage(&mClipper);
	mClipper.setNextStage(&mDivider);
	mDivider.setNextStage(&mCuller);
	mCuller.setNextStage(&mMapper);
	mMapper.setNextStage(&mRast);
}

void DrawEngine::validateState(DrawContext *dc)
{
	// TODO: some validation work
	GLContext *gc = dc->gc;
	VertexArrayObject *pVAO = gc->mVAOM.getActiveVAO();
	BufferObject *pElementBO = gc->mBO.getBoundBuffer(GL_ELEMENT_ARRAY_BUFFER))

	if(dc->mDrawType == DrawContext::kArrayDraw)
	{
	}
	else if(dc->mDrawType == DrawContext::kElementDraw)
	{
		if(!pElementBO && !dc->mIndices)
			return;
	}
}

DrawContext * DrawEngine::prapareDrawContext(GLContext *gc)
{
	DrawContext *dc = new DrawContext();
}

void DrawEngine::emit(DrawContext *dc)
{
	getFirstStage()->emit(dc);
}

DrawEngine DrawEngine::DE;