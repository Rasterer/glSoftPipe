#include <cstring>
#include <iostream>
#include "PrimitiveProcessor.h"

// TODO: unify the vertex data struct in all pipe stages
// TODO: Two design thoughts:
// 1. process all primitives and then next stage. (current)
// 2. process one (or a batch of)  primitives and then pass to next stage.
void PrimitiveContext::calVertexSize()
{
	size_t size = 0;

	for(size_t i = 0; i < attriNum; i++)
		size += elementSize[i];

	sizePerVertex = size * sizeof(float);
}

vertex *PrimitiveContext::poolAlloc(size_t n)
{
	std::cout << "jzb: calVertexSize  " << (sizeof(vertex) + sizePerVertex) << std::endl;
	return (vertex *)malloc((sizeof(vertex) + sizePerVertex) * n);
}

void PrimitiveContext::poolFree(vertex *pVert)
{
	free(pVert);
}

vertex *PrimitiveContext::nextVertex(vertex *pVert)
{
	return (vertex *)((char *)pVert + sizeof(vertex) + sizePerVertex);
}

void PrimitiveContext::fetchVertex(vertex &vert, int idx)
{
	float *tmp = vert.reg;
	for(size_t i = 0; i < attriNum; i++)
	{
		memcpy((char *)tmp, (char *)((float *)vertexAttri[i] + idx * elementSize[i]), elementSize[i] * sizeof(float));
		tmp += elementSize[i];
	}
}

void PrimitiveContext::lerp(vertex &vert, int idx0, int idx1, float t)
{
	float *tmp = vert.reg;

	for(size_t i = 0; i < attriNum; i++)
	{
		for(size_t j = 0; j < elementSize[i]; j++)
		{
			*tmp = *((float *)vertexAttri[i] + idx0 * elementSize[i] + j) * (1.0 - t) +
				   *((float *)vertexAttri[i] + idx1 * elementSize[i] + j) * t;

			std::cout << "jzb: lerp result " << *tmp << std::endl;
			tmp++;
		}
	}
}

void PrimitiveContext::writeBackVertex(vertex *pVert, int *idx, int n, int writePos)
{
	for(size_t i = 0; i < n; i++)
	{
		if(idx[i] >= indexBufferSize)
		{
			int p = 0;
			for(size_t j = 0; j < attriNum; j++)
			{
				for(size_t k = 0; k < elementSize[j]; k++)
				{
					//std::cout << "jzb: writeBackVertex " << pVert->reg[p] << std::endl;
					*((float *)vertexAttri[j] + writePos * elementSize[j] + k) = pVert->reg[p];
					p++;
				}
			}

			writePos++;
		}
		pVert = nextVertex(pVert);
	}
}

PrimitiveProcessor::PrimitiveProcessor():
	mCtx(NULL),
	mCullEnabled(true),
	mCullFace(BACK),
	mFrontFaceOrient(CCW)
{
	mClipPlanes[0] = vec4(0.0f, 0.0f, 1.0f, 1.0);	// near plane
	mClipPlanes[1] = vec4(0.0f, 0.0f, -1.0f, 1.0);	// far  plane
	mClipPlanes[2] = vec4(1.0f, 0.0f, 0.0f, 1.0);	// left plane
	mClipPlanes[3] = vec4(-1.0f, 0.0f, 0.0f, 1.0);	// right plane
	mClipPlanes[4] = vec4(0.0f, 1.0f, 0.0f, 1.0);	// bottom plane
	mClipPlanes[5] = vec4(0.0f, -1.0f, 0.0f, 1.0);	// top plane
}

void PrimitiveProcessor::attachContext(PrimitiveContext *ctx)
{
	mCtx = ctx;
	mCtx->calVertexSize();
}

void PrimitiveProcessor::detachContext()
{
	mCtx = NULL;
}

void PrimitiveProcessor::setupViewport(unsigned x, unsigned y, unsigned w, unsigned h)
{
	mViewport.x = x;
	mViewport.y = y;
	mViewport.w = w;
	mViewport.h = h;
}

void PrimitiveProcessor::run()
{
	primitiveAssembly();
	clip();
	perspectiveDivide();
	viewportTransform();
	cull();
}

void PrimitiveProcessor::primitiveAssembly()
{
	// TODO: account for strip & fan mode triangles
	// TODO: account for point & line primitives.
	// After clipping, there can be at most 6 vertices per triangle.
	// So make room for this newly produced vertices here.
	mCtx->mOutIB = (int *)malloc(sizeof(int) * mCtx->indexBufferSize * 4);
	//memcpy(mIntermIB, mCtx->indexBuffer, mCtx->indexBufferSize);
}

// FIXME: account for clip coord (0, 0, 0, 0)
// TODO: guard-band clipping
void PrimitiveProcessor::clip()
{
	vec4 *clipCoor = (vec4 *)mCtx->vertexAttri[POSITION_INDEX];
	vertex *pVert = mCtx->poolAlloc(6);
	int newIBSize = 0;
	int newVBSize = mCtx->vertexCount;

	for(size_t i = 0; i < mCtx->indexBufferSize; i += 3)
	{
		int srcVertexNum = 3;
		int dstVertexNum = 0;
		int srcIndex[6] = {
			mCtx->indexBuffer[i+0],
			mCtx->indexBuffer[i+1],
			mCtx->indexBuffer[i+2],
		};
		int dstIndex[6];
		float dist[2];

		std::cout << "jzb: here: " << i << std::endl;
		for(size_t j = 0; j < ARRAY_SIZE(mClipPlanes); j++)
		{
			int tmpAdvance = newVBSize;
			vertex *pVerTmp = pVert;

			if(srcVertexNum > 0)
				dist[0] = dot(*(clipCoor + srcIndex[0]), mClipPlanes[j]);

			for(size_t k = 1; k <= srcVertexNum; k++)
			{
				dist[1] = dot(*(clipCoor + srcIndex[k % srcVertexNum]), mClipPlanes[j]);

				std::cout << "jzb:dist0: " << dist[0] << " dist1: " << dist[1] << " " << k << std::endl;
				// Can not use unified linear interpolation equation.
				// Otherwise, if clip AB and BA, the results will be different.
				// Here we solve this by clip an edge in a fixed direction: from inside out
				if(dist[0] >= 0.0f)
				{
					std::cout << "jzb: fetchVertex: " << j << " " << newVBSize << " " << dstVertexNum << std::endl;
					mCtx->fetchVertex(*pVerTmp, srcIndex[k - 1]);

					if(srcIndex[k - 1] < newVBSize)
					{
						dstIndex[dstVertexNum] = srcIndex[k - 1];
					}
					else
					{
						dstIndex[dstVertexNum] = tmpAdvance;
						tmpAdvance++;
					}
					dstVertexNum++;
					pVerTmp = mCtx->nextVertex(pVerTmp);

					if(dist[1] < 0.0f)
					{
						mCtx->lerp(*pVerTmp, srcIndex[k - 1], srcIndex[k % srcVertexNum], dist[0] / (dist[0] - dist[1]));
						dstIndex[dstVertexNum] = tmpAdvance;
						dstVertexNum++;
						tmpAdvance++;
						pVerTmp = mCtx->nextVertex(pVerTmp);
					}
				}
				else if(dist[1] >= 0.0f)
				{
					mCtx->lerp(*pVerTmp, srcIndex[k % srcVertexNum], srcIndex[k - 1], dist[1] / (dist[1] - dist[0]));
					dstIndex[dstVertexNum] = tmpAdvance;
					dstVertexNum++;
					tmpAdvance++;
					pVerTmp = mCtx->nextVertex(pVerTmp);
				}

#if 0
				if(DIFFERENT_SIGNS(dist[0], dist[1]))
				{
					std::cout << "jzb: call lerp " << j << " " << newVBSize << " " << dstVertexNum << std::endl;
					mCtx->lerp(*pVerTmp, srcIndex[k - 1], srcIndex[k % srcVertexNum], dist[0] / (dist[0] - dist[1]));
					dstIndex[dstVertexNum] = tmpAdvance;
					dstVertexNum++;
					tmpAdvance++;
					pVerTmp = mCtx->nextVertex(pVerTmp);
				}
#endif

				dist[0] = dist[1];
			}
			std::cout << "jzb: here1: " << j << " " << newVBSize << " " << dstVertexNum << std::endl;

			srcVertexNum = dstVertexNum;

			if(srcVertexNum == 0)
			{
				break;
			}
			mCtx->writeBackVertex(pVert, dstIndex, dstVertexNum, newVBSize);

			for(size_t k = 0; k < dstVertexNum; k++)
				srcIndex[k] = dstIndex[k];

			dstVertexNum = 0;
		}

		if(srcVertexNum == 0)
		{
			continue;
		}

		{	// Advance the VB
			int count = 0;

			for(size_t j = 0; j < srcVertexNum; j++)
			{
				if(srcIndex[j] >= newVBSize)
				{
					count++;
				}
			}
			newVBSize += count;
		}

		{	// Triangulation
			//std::cout << "jzb: here2: " << i << " " << newIBSize << " " << srcVertexNum << std::endl;
			int start = srcIndex[0];
			for(size_t j = 1; j < srcVertexNum - 1; j++)
			{
				mCtx->mOutIB[newIBSize++] = start;
				mCtx->mOutIB[newIBSize++] = srcIndex[j];
				mCtx->mOutIB[newIBSize++] = srcIndex[j+1];
			}
		}
	}

	mCtx->mOutIBSize = newIBSize;
	mCtx->mOutVBSize = newVBSize;
	mCtx->poolFree(pVert);
	std::cout << "jzb: new IB size " << newIBSize << std::endl;
	std::cout << "jzb: new VB size " << newVBSize << std::endl;
	//std::cout << "new vertex x3 " << ((vec4 *)mCtx->vertexAttri[0] + 3)->x << std::endl;
	//std::cout << "new vertex y3 " << ((vec4 *)mCtx->vertexAttri[0] + 3)->y << std::endl;
	//std::cout << "new vertex x4 " << ((vec4 *)mCtx->vertexAttri[0] + 4)->x << std::endl;
	//std::cout << "new vertex y4 " << ((vec4 *)mCtx->vertexAttri[0] + 4)->y << std::endl;
}

void PrimitiveProcessor::perspectiveDivide()
{
	for(size_t i = 0; i < mCtx->mOutVBSize; i++)
	{
		vec4 &pos = *((vec4 *)mCtx->vertexAttri[POSITION_INDEX] + i);
		std::cout << "jzb: " << i << " before pers divide x " << pos.x << std::endl;
		std::cout << "jzb: " << i << " before pers divide y " << pos.y << std::endl;
		std::cout << "jzb: " << i << " before pers divide z " << pos.z << std::endl;
		std::cout << "jzb: " << i << " before pers divide w " << pos.w << std::endl;

		pos.x /= pos.w;
		pos.y /= pos.w;
		pos.z /= pos.w;
	}
}

void PrimitiveProcessor::viewportTransform()
{
	for(size_t i = 0; i < mCtx->mOutVBSize; i++)
	{
		vec4 &pos = *((vec4 *)mCtx->vertexAttri[POSITION_INDEX] + i);

		pos.x = (pos.x + 1.0f) * mViewport.w / 2.0f + mViewport.x;
		pos.y = (pos.y + 1.0f) * mViewport.h / 2.0f + mViewport.y;
	}
}

void PrimitiveProcessor::cull()
{
	size_t count = 0;
	int *intermeBuffer = (int *)malloc(mCtx->mOutIBSize * sizeof(int));

	for(size_t i = 0; i < mCtx->mOutIBSize; i += 3)
	{
		vec4 &pos0 = *((vec4 *)mCtx->vertexAttri[POSITION_INDEX] + mCtx->mOutIB[i+0]);
		vec4 &pos1 = *((vec4 *)mCtx->vertexAttri[POSITION_INDEX] + mCtx->mOutIB[i+1]);
		vec4 &pos2 = *((vec4 *)mCtx->vertexAttri[POSITION_INDEX] + mCtx->mOutIB[i+2]);

		float ex = pos1.x - pos0.x;
		float ey = pos1.y - pos0.y;
		float fx = pos2.x - pos0.x;
		float fy = pos2.y - pos0.y;
		float area = ex * fy - ey * fx;
		//std::cout << "jzb: viewport0 x " << pos2.x << std::endl;
		//std::cout << "jzb: viewport0 y " << pos2.y << std::endl;

		if(!EQUAL(area, 0.0f))
		{
			orient_t orient = (area > 0)? CCW: CW;
			face_t face = (mFrontFaceOrient == orient)? FRONT: BACK;

			if(!mCullEnabled || (mCullFace & face) == 0)
			{
				intermeBuffer[count++] = mCtx->mOutIB[i+0];
				intermeBuffer[count++] = mCtx->mOutIB[i+1];
				intermeBuffer[count++] = mCtx->mOutIB[i+2];
			}
		}
	}

	free(mCtx->mOutIB);
	mCtx->mOutIB = intermeBuffer;
	mCtx->mOutIBSize = count;
}
