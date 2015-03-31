#include <iostream>
#include "VertexCache.h"
#include "DrawEngine.h"
#include "glsp_defs.h"

VertexCachedAssembler::VertexCachedAssembler()
{
}

void VertexCachedAssembler::emit(void *data)
{
	GLSP_UNREFERENCED_PARAM(data);

	DrawContext *dc = getDrawCtx();
	GLContext *gc = dc->gc;

	assembleVertex(dc, gc);
}

void VertexCachedAssembler::assembleVertex(DrawContext *dc, GLContext *gc)
{
	assert(mIndexType == GL_UNSIGNED_INT);

	int *idx = static_cast<int *>(dc->mIndices);
	VertexArrayObject *pVAO = gc->mVAOM.getActiveVAO();
	Batch * bat = NULL;

	// OPT: Too many copies 
	for(int i = 0, id = 0; i < dc->mCount; i++)
	{
		int i0 = idx[dc->mStart + i];
		vsInput in;

		if(!bat)
		{
			bat = new Batch();
		}

		vsCache & cache = bat->mCache;

		for(int j = 0; j < MAX_VERTEX_ATTRIBS; j++)
		{
			vec4 attrib(0.0f, 0.0f, 0.0f, 1.0f);

			if(pVAO->mAttribEnables & (1 < j))
			{
				BufferObject *pBO;
				const VertexAttribState &vas = pVAO->mAttribState[j];
				int stride = vas.mStride ? vas.mStride: vas.mAttribSize;
				const char *src;

				if((pBO = vas.mBO) != NULL)
				{
					src = static_cast<char *>(pBO->mAddr);
					src = src + stride * i0 + vas.mOffset;
				}
				else
				{
					src = reinterpret_cast<char *>(vas.mOffset);
					src = src + stride * i0;
				}
				memcpy(&attrib, src, vas.mAttribSize);
			}
			else // TODO: Impl accessing non-enabled attrib
			{
			}
			
			in.assemble(attrib);
		}
		
		{
			vsCache::iterator it = cache.find(in);
			
			if(it != cache.end())
			{
				bat->mIndex.push_back(it->second);
			}
			else
			{
				cache[in] = id;
				bat->mIndex.push_back(id);
			}
		}
		cache[in] = id;
		id = cache.size();

		if((i % 3 == 2) && (id >= VERTEX_CACHE_EMIT_THRESHHOLD))
		{
			getNextStage()->emit(bat);
			bat = NULL;
		}
	}

	if(bat)
	{
		getNextStage()->emit(bat);
	}
}
