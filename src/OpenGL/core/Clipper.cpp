#include "Clipper.h"

#include <utility>

#include "DataFlow.h"
#include "DrawEngine.h"
#include "MemoryPool.h"
#include "compiler.h"


namespace glsp {

using glm::vec4;



static glm::vec4 sPlanes[Clipper::MAX_PLANES] = {
	glm::vec4( 0.0f,  0.0f,  1.0f, 1.0f), // PLANE_NEAR
	glm::vec4( 0.0f,  0.0f, -1.0f, 1.0f), // PLANE_FAR
	glm::vec4( 1.0f,  0.0f,  0.0f, 1.0f), // PLANE_LEFT
	glm::vec4(-1.0f,  0.0f,  0.0f, 1.0f), // PLANE_RIGHT
	glm::vec4( 0.0f,  1.0f,  0.0f, 1.0f), // PLANE_BOTTOM
	glm::vec4( 0.0f, -1.0f,  0.0f, 1.0f), // PLANE_TOP
	glm::vec4( 0.0f,  0.0f,  0.0f, 1.0f), // PLANE_ZEROW
	glm::vec4( 1.0f,  0.0f,  0.0f, 0.0f), // PLANE_GB_LEFT
	glm::vec4(-1.0f,  0.0f,  0.0f, 0.0f), // PLANE_GB_RIGHT
	glm::vec4( 0.0f,  1.0f,  0.0f, 0.0f), // PLANE_GB_BOTTOM
	glm::vec4( 0.0f, -1.0f,  0.0f, 0.0f)  // PLANE_GB_TOP
};

Clipper::Clipper():
	PipeStage("Clipping", DrawEngine::getDrawEngine())
{
}

void Clipper::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);

	onClipping(bat);

	getNextStage()->emit(bat);
}

// Compute Cohen-Sutherland style outcodes against the view frustum
void Clipper::ComputeOutcodesFrustum(const Primitive &prim, int outcodes[3])
{
	float dist[3];

	for (int i = PLANE_NEAR; i <= PLANE_ZEROW; ++i)
	{
		dist[0] = glm::dot(prim.mVert[0].position(), sPlanes[i]);
		dist[1] = glm::dot(prim.mVert[1].position(), sPlanes[i]);
		dist[2] = glm::dot(prim.mVert[2].position(), sPlanes[i]);

		if (i == PLANE_ZEROW)
		{
			// Plane w > 0, use "<=" to get rid of clip space (0, 0, 0, 0),
			// which is a degenerate triangle and can be thrown way directly
			if (dist[0] <= 0.0f)	outcodes[0] |= (1 << i);
			if (dist[1] <= 0.0f)	outcodes[1] |= (1 << i);
			if (dist[2] <= 0.0f)	outcodes[2] |= (1 << i);
		}
		else
		{
			if (dist[0] < 0.0f)		outcodes[0] |= (1 << i);
			if (dist[1] < 0.0f)		outcodes[1] |= (1 << i);
			if (dist[2] < 0.0f)		outcodes[2] |= (1 << i);
		}
	}
}

// Compute outcodes against the guard band
void Clipper::ComputeOutcodesGuardband(const Primitive &prim, int outcodes[3])
{
	float dist[3];

	for (int i = PLANE_GB_LEFT; i <= PLANE_GB_TOP; ++i)
	{
		dist[0] = glm::dot(prim.mVert[0].position(), sPlanes[i]);
		dist[1] = glm::dot(prim.mVert[1].position(), sPlanes[i]);
		dist[2] = glm::dot(prim.mVert[2].position(), sPlanes[i]);

		if (dist[0] < 0.0f)		outcodes[0] |= (1 << i);
		if (dist[1] < 0.0f)		outcodes[1] |= (1 << i);
		if (dist[2] < 0.0f)		outcodes[2] |= (1 << i);
	}
}


/* NOTE:
 * When guard band is used, need clip against guard band
 * rather than view frustum. Otherwise, there will be cracks
 * on the shared edge between two triangles(one is inside
 * guard band, while another intersects with guard band)
 * after snapping to subpixel grids.
 */
void Clipper::ClipAgainstGuardband(Primitive &prim, int outcodes_union, Primlist &out)
{
	/* Two round robin intermediate boxes
	 * One src, one dst. Next loop, reverse!
	 */
	vsOutput tmp[6 * 2];
	vsOutput *rr[2][6];
	int vertNum[2];
	float dist[2];
	int src = 0, dst = 1;
	int tmpnr = 0;

	assert(prim.mType == Primitive::TRIANGLE);

	rr[src][0] = &(prim.mVert[0]);
	rr[src][1] = &(prim.mVert[1]);
	rr[src][2] = &(prim.mVert[2]);

	vertNum[src] = 3;

	unsigned long i;
	while (_BitScanForward(&i, (unsigned long)outcodes_union))
	{
		outcodes_union &= ~(1 << i);

		if(vertNum[src] > 0)
			dist[0] = glm::dot(rr[src][0]->position(), sPlanes[i]);

		vertNum[dst] = 0;

		for(int j = 0; j < vertNum[src]; j++)
		{
			int k = (j + 1) % vertNum[src];
			dist[1] = dot(rr[src][k]->position(), sPlanes[i]);

			// Can not use unified linear interpolation equation.
			// Otherwise, if clip AB and BA, the results will be different.
			// Here we solve this by clip an edge in a fixed direction: from inside out
			if(dist[0] >= 0.0f)
			{
				rr[dst][vertNum[dst]++] = rr[src][j];

				if(dist[1] < 0.0f)
				{
					vsOutput *new_vert = &tmp[tmpnr++];

					vertexLerp(*new_vert, *rr[src][j], *rr[src][k], dist[0] / (dist[0] - dist[1]));
					rr[dst][vertNum[dst]++] = new_vert;
				}
			}
			else if(dist[1] >= 0.0f)
			{
				vsOutput *new_vert = &tmp[tmpnr++];

				vertexLerp(*new_vert, *rr[src][k], *rr[src][j], dist[1] / (dist[1] - dist[0]));
				rr[dst][vertNum[dst]++] = new_vert;
			}

			dist[0] = dist[1];
		}

		src = (src + 1) & 0x1;
		dst = (dst + 1) & 0x1;
	}

	if(vertNum[src] > 0)
	{
		assert(vertNum[src] >= 3 && vertNum[src] <= 6);

		// Triangulation
		for (int i = 1; i < vertNum[src] - 1; i++)
		{
			Primitive *new_prim = new(MemoryPoolMT::get()) Primitive;

			new_prim->mType    = Primitive::TRIANGLE;
			new_prim->mVertNum = 3;
			new_prim->mVert[0] = *rr[src][0];
			new_prim->mVert[1] = std::move(*rr[src][i]); /* OPT to avoid copy */
			new_prim->mVert[2] = *rr[src][i+1];
			new_prim->mRasterStates = prim.mRasterStates;

			out.push_back(new_prim);
		}
	}
}

void Clipper::onClipping(Batch *bat)
{
	Primlist out;
	Primlist &pl = bat->mPrims;

	auto it = pl.begin();
	while (it != pl.end())
	{
		int   outcodes[3] = {0, 0, 0};
		Primitive *prim = *it;

		ComputeOutcodesFrustum(*prim, outcodes);

		// trivially accepted
		if ((outcodes[0] | outcodes[1] | outcodes[2]) == 0)
		{
			out.push_back(prim);
			it = pl.erase(it);

			continue;
		}
		auto orig_it = it;
		++it;

		// trivially rejected
		if (outcodes[0] & outcodes[1] & outcodes[2])
		{
			continue;
		}

		/* A bit tricky here:
		 * Consider this particular outcode b1000000:
		 * -w <= x <= w (true)
		 * -w <= y <= w (true)
		 * -w <= z <= w (true)
		 * w > 0        (false)
		 *
		 * This can derive that (x, y, z, w) = (0, 0, 0, 0).
		 * So it's an efficient way to catch this degenerated case.
		 */
		if (UNLIKELY(outcodes[0] == (1 << PLANE_ZEROW) ||
					 outcodes[1] == (1 << PLANE_ZEROW) ||
					 outcodes[2] == (1 << PLANE_ZEROW)))
		{
			continue;
		}

		ComputeOutcodesGuardband(*prim, outcodes);

		unsigned outcodes_union = (outcodes[0] | outcodes[1] | outcodes[2]) & kGBClipMask;

		if (LIKELY(outcodes_union == 0))
		{
			out.push_back(prim);
			it = pl.erase(orig_it);
		}
		else
		{
			// need to do clipping
			ClipAgainstGuardband(*prim, outcodes_union, out);
		}
	}

	pl.swap(out);

	for (Primitive *prim: out)
	{
		prim->~Primitive();
		MemoryPoolMT::get().deallocate(prim, sizeof(Primitive));
	}
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

void Clipper::ComputeGuardband(float width, float height)
{
	sPlanes[PLANE_GB_LEFT  ].w = sPlanes[PLANE_GB_RIGHT ].w = GUARDBAND_WIDTH  / width;
	sPlanes[PLANE_GB_BOTTOM].w = sPlanes[PLANE_GB_TOP   ].w = GUARDBAND_HEIGHT / height;
}

void Clipper::finalize()
{
}

} // namespace glsp
