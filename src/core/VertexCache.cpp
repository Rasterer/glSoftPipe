#include <iostream>
#include "VertexCache.h"
#include "DataFlow.h"
#include "GLContext.h"
#include "DrawEngine.h"
#include "VertexArrayObject.h"
#include "glsp_defs.h"

VertexCachedAssembler::VertexCachedAssembler():
	PipeStage("Vertex Assembly", DrawEngine::getDrawEngine())
{
}

void VertexCachedAssembler::emit(void *data)
{
	DrawContext *dc = static_cast<DrawContext *>(data);

	assembleVertex(dc);
}

// Pre T&L cache implementation
// TODO: need post T&L cache ?
void VertexCachedAssembler::assembleVertex(DrawContext *dc)
{
	GLContext *gc = dc->gc;
	const unsigned int *iBuf = static_cast<const unsigned int *>(dc->mIndices);
	VertexArrayObject *pVAO = gc->mVAOM.getActiveVAO();
	VertexShader *pVS = gc->mPM.getCurrentProgram()->getVS();
	BufferObject *pIBO = gc->mBOM.getBoundBuffer(GL_ELEMENT_ARRAY_BUFFER);
	Batch * bat = NULL;

	assert(dc->mIndexSize == sizeof(unsigned int));

	if(pIBO)
		iBuf = (unsigned int *)((char *)pIBO->mAddr + (ptrdiff_t)iBuf);

	// OPT: Too many copies 
	for(int i = 0; i < dc->mCount; i++)
	{
		unsigned int idx = iBuf[dc->mFirst + i];

		if(!bat)
		{
			bat = new Batch();
			bat->mDC = dc;
		}

		vsCache & cache = bat->mVertexCache;
		vsCacheIndex & cacheIndex = bat->mCacheIndex;
		vsCacheIndex::iterator it = cacheIndex.find(idx);

		if(it != cacheIndex.end())
		{
			bat->mIndexBuf.push_back(it->second);
		}
		else
		{
			vsInput in;

			in.reserve(pVS->getInRegsNum());

			for(size_t j = 0; j < pVS->getInRegsNum(); j++)
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
						src = src + stride * idx + vas.mOffset;
					}
					else
					{
						src = reinterpret_cast<char *>(vas.mOffset);
						src = src + stride * idx;
					}
					memcpy(&attrib, src, vas.mAttribSize);
				}
				else // TODO: Impl accessing non-enabled attributes
				{
				}
				
				in.pushReg(attrib);
			}

			cacheIndex[idx] = cache.size();
			bat->mIndexBuf.push_back(cache.size());
			cache.push_back(in);
		}

		// Check if it's time to dispatch this batch
		if((i % 3 == 2) && (cache.size() >= VERTEX_CACHE_EMIT_THRESHHOLD))
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

void VertexCachedAssembler::finalize()
{
}
