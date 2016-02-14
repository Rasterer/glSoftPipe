#include "VertexFetcher.h"

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <utility>

#include "DataFlow.h"
#include "GLContext.h"
#include "DrawEngine.h"
#include "VertexArrayObject.h"
#include "ThreadPool.h"
#include "khronos/GL/glspcorearb.h"

namespace glsp {


using glm::vec4;

VertexFetcher::VertexFetcher():
	PipeStage("Vertex Fetching", DrawEngine::getDrawEngine())
{
}

VertexCachedFetcher::VertexCachedFetcher():
	VertexFetcher()
{
}

void VertexCachedFetcher::emit(void *data)
{
	DrawContext *dc = static_cast<DrawContext *>(data);

	FetchVertex(dc);
}

// Pre shading cache implementation
// OPT: Is post shading cache better?
void VertexCachedFetcher::FetchVertex(DrawContext *dc)
{
	::glsp::ThreadPool &thread_pool = ::glsp::ThreadPool::get();

// NOTE: need to be multiple of 3.
#define VERTEX_INDEX_STEP 66
	for (int v = 0; v < dc->mCount; v += VERTEX_INDEX_STEP)
	{
		auto vert_batch_handler = [this, v](void *data)
		{
			int i = v;
			DrawContext *dc = static_cast<DrawContext *>(data);
			GLContext *gc = dc->gc;

			// First int is the vertex index from IBO.
			// Second int is the vertex index in vertex cache.

			const unsigned int *iBuf = static_cast<const unsigned int *>(dc->mIndices);

			VertexArrayObject *pVAO = gc->mVAOM.getActiveVAO();
			VertexShader      *pVS  = gc->mPM.getCurrentProgram()->getVS();
			BufferObject      *pIBO = gc->mBOM.getBoundBuffer(GL_ELEMENT_ARRAY_BUFFER);

			if(dc->mDrawType == DrawContext::kElementDraw)
				// TODO: add other index type support
				assert(dc->mIndexSize == sizeof(unsigned int));

			if(pIBO)
				iBuf = (unsigned int *)((uintptr_t)pIBO->mAddr + (ptrdiff_t)iBuf);

			std::unordered_map<int, int> cacheIndex;
			Batch bat;
			bat.mDC = dc;
			vsInput_v &cache = bat.mVertexCache;

			// OPT: Too many copies 
			for (int j = 0; i < dc->mCount && j < VERTEX_INDEX_STEP; ++i, ++j)
			{
				unsigned int idx = 0;

				if(dc->mDrawType == DrawContext::kElementDraw)
					idx = iBuf[dc->mFirst + i];
				else if(dc->mDrawType == DrawContext::kArrayDraw)
					idx = i;
				else
					assert(false);

				auto it = cacheIndex.find(idx);

				if(it != cacheIndex.end())
				{
					bat.mIndexBuf.push_back(it->second);
				}
				else
				{
					vsInput in;

					in.resize(pVS->getInRegsNum());

					for(size_t j = 0; j < pVS->getInRegsNum(); j++)
					{
						if(pVAO->mAttribEnables & (1 << j))
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
							std::memcpy((void *)((uintptr_t)(in.data()) + j * sizeof(glm::vec4)), src, vas.mAttribSize);
						}
						else // TODO: Impl accessing non-enabled attributes
						{
						}
					}

					cacheIndex[idx] = cache.size();
					bat.mIndexBuf.push_back(cache.size());
					cache.push_back(std::move(in));
				}
			}

			this->getNextStage()->emit(&bat);
		};
		WorkItem *task = thread_pool.CreateWork(vert_batch_handler, dc);
		thread_pool.AddWork(task);
	}
}

void VertexCachedFetcher::finalize()
{
}

} // namespace glsp
