#include "Clipper.h"
#include "DataFlow.h"
#include "DrawEngine.h"
#include "utils.h"


NS_OPEN_GLSP_OGL()

using namespace std;
using namespace glm;

Clipper::Clipper():
	PipeStage("Clipping", DrawEngine::getDrawEngine())
{
	mPlanes[0] = vec4(0.0f, 0.0f, 1.0f, 1.0);	// near plane
	mPlanes[1] = vec4(0.0f, 0.0f, -1.0f, 1.0);	// far  plane
	mPlanes[2] = vec4(1.0f, 0.0f, 0.0f, 1.0);	// left plane
	mPlanes[3] = vec4(-1.0f, 0.0f, 0.0f, 1.0);	// right plane
	mPlanes[4] = vec4(0.0f, 1.0f, 0.0f, 1.0);	// bottom plane
	mPlanes[5] = vec4(0.0f, -1.0f, 0.0f, 1.0);	// top plane
}

void Clipper::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);

	clipping(bat);

	getNextStage()->emit(bat);
}

// FIXME: account for clip coord (0, 0, 0, 0)
// TODO: guard-band clipping
void Clipper::clipping(Batch *bat)
{
	PrimBatch out;
	PrimBatch &in = bat->mPrims;
#if PRIMITIVE_OWNS_VERTICES
	vsOutput tmp[ARRAY_SIZE(mPlanes) * 3];
#elif PRIMITIVE_REFS_VERTICES
	unordered_set<vsOutput *> need_free;
#endif

	for(auto it = in.begin(); it != in.end(); it++)
	{
		// Two round robin intermediate boxes
		// One src, one dst
		// Next loop, switch!
		vsOutput *rr[2][6];
		int vertNum[2];
		float dist[2];
		int src = 0, dst = 1;
		int tmpnr = 0;

		assert(it->mType == Primitive::TRIANGLE);

		rr[src][0] = &(it->mVert[0]);
		rr[src][1] = &(it->mVert[1]);
		rr[src][2] = &(it->mVert[2]);

		vertNum[src] = 3;
		vertNum[dst] = 0;

		for(size_t i = 0; i < ARRAY_SIZE(mPlanes); i++)
		{
			if(vertNum[src] > 0)
				dist[0] = dot(rr[src][0]->position(), mPlanes[i]);

			vertNum[dst] = 0;

			for(int j = 0; j < vertNum[src]; j++)
			{
				int k = (j + 1) % vertNum[src];
				dist[1] = dot(rr[src][k]->position(), mPlanes[i]);

				// Can not use unified linear interpolation equation.
				// Otherwise, if clip AB and BA, the results will be different.
				// Here we solve this by clip an edge in a fixed direction: from inside out
				if(dist[0] >= 0.0f)
				{
					rr[dst][vertNum[dst]] = rr[src][j];
					++vertNum[dst];

					if(dist[1] < 0.0f)
					{
#if PRIMITIVE_OWNS_VERTICES
						vsOutput *new_vert = &tmp[tmpnr++];
#elif PRIMITIVE_REFS_VERTICES
						vsOutput *new_vert = new vsOutput(*rr[src][j]);
#endif
						vertexLerp(*new_vert, *rr[src][j], *rr[src][k], dist[0] / (dist[0] - dist[1]));
						rr[dst][vertNum[dst]] = new_vert;
						++vertNum[dst];
					}
				}
				else if(dist[1] >= 0.0f)
				{
#if PRIMITIVE_OWNS_VERTICES
					vsOutput *new_vert = &tmp[tmpnr++];
#elif PRIMITIVE_REFS_VERTICES
					need_free.insert(rr[src][j]);
					vsOutput *new_vert = new vsOutput(*rr[src][j]);
#endif
					vertexLerp(*new_vert, *rr[src][k], *rr[src][j], dist[1] / (dist[0] - dist[1]));
					rr[dst][vertNum[dst]] = new_vert;
					++vertNum[dst];
				}

				dist[0] = dist[1];
			}

			src = (src + 1) & 0x1;
			dst = (dst + 1) & 0x1;
		}

		if(vertNum[src] == 0)
			continue;

		assert(vertNum[src] >= 3 && vertNum[src] <= 6);
		
		// Triangulation
		for(int i = 1; i < vertNum[src] - 1; i++)
		{
			Primitive prim;

			prim.mType = Primitive::TRIANGLE;
#if PRIMITIVE_OWNS_VERTICES
			prim.mVert[0] = *rr[src][0];
			prim.mVert[1] = *rr[src][i];
			prim.mVert[2] = *rr[src][i+1];
#elif PRIMITIVE_REFS_VERTICES
			prim.mVert[0] = rr[src][0];
			prim.mVert[1] = rr[src][i];
			prim.mVert[2] = rr[src][i+1];
#endif
			out.push_back(prim);
		}
	}

#if PRIMITIVE_REFS_VERTICES
	// OPT: more decent with smart pointer or ref count?
	for(auto iter = need_free.begin(); iter != need_free.end(); ++iter)
	{
		delete (*iter);
	}
#endif

	in.swap(out);
}

// vertex linear interpolation
void Clipper::vertexLerp(vsOutput &new_vert,
		  vsOutput &vert1,
		  vsOutput &vert2,
		  float t)
{
	assert(vert1.getRegsNum() == vert2.getRegsNum());

	new_vert.resize(vert1.getRegsNum());

	for(size_t i = 0; i < vert1.getRegsNum(); ++i)
	{
		new_vert.getReg(i) = vert1.getReg(i) * (1 - t)  + vert2.getReg(i) * t;
	}
}

void Clipper::finalize()
{
}

NS_CLOSE_GLSP_OGL()
