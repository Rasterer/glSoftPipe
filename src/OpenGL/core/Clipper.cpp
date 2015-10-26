#include "Clipper.h"
#include "DataFlow.h"
#include "DrawEngine.h"
#include "utils.h"


NS_OPEN_GLSP_OGL()

using glm::vec4;

Clipper::Clipper():
	PipeStage("Clipping", DrawEngine::getDrawEngine())
{
	mPlanes[0] = vec4(0.0f, 0.0f, 1.0f, 1.0f);	// near plane
	mPlanes[1] = vec4(0.0f, 0.0f, -1.0f, 1.0f);	// far  plane
	mPlanes[2] = vec4(1.0f, 0.0f, 0.0f, 1.0f);	// left plane
	mPlanes[3] = vec4(-1.0f, 0.0f, 0.0f, 1.0f);	// right plane
	mPlanes[4] = vec4(0.0f, 1.0f, 0.0f, 1.0f);	// bottom plane
	mPlanes[5] = vec4(0.0f, -1.0f, 0.0f, 1.0f);	// top plane
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
	Primlist out;
	Primlist &pl = bat->mPrims;
	vsOutput tmp[ARRAY_SIZE(mPlanes) * 4];

	for(auto it = pl.begin(); it != pl.end(); it++)
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
						vsOutput *new_vert = &tmp[tmpnr++];

						vertexLerp(*new_vert, *rr[src][j], *rr[src][k], dist[0] / (dist[0] - dist[1]));
						rr[dst][vertNum[dst]] = new_vert;
						++vertNum[dst];
					}
				}
				else if(dist[1] >= 0.0f)
				{
					vsOutput *new_vert = &tmp[tmpnr++];

					vertexLerp(*new_vert, *rr[src][k], *rr[src][j], dist[1] / (dist[1] - dist[0]));
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

		assert(vertNum[src] >= 3 && vertNum[src] <= 7);

		// Triangulation
		for(int i = 1; i < vertNum[src] - 1; i++)
		{
			Primitive prim;

			prim.mType    = Primitive::TRIANGLE;
			prim.mVertNum = 3;
			prim.mVert[0] = *rr[src][0];
			prim.mVert[1] = *rr[src][i];
			prim.mVert[2] = *rr[src][i+1];
			prim.mDC      = bat->mDC;

			out.push_back(std::move(prim));
		}
	}

	pl.swap(out);
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
		new_vert[i] = vert1[i] * (1 - t)  + vert2[i] * t;
	}
}

void Clipper::finalize()
{
}

NS_CLOSE_GLSP_OGL()
