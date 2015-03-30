#include "Rasterizer.h"
#include "utils.h"

int Rasterizer::setupInput(size_t vertexCount, size_t varyingNum, void *varyingPtr[], size_t *elementSize, int *indexBuffer, size_t indexBufferSize, FragmentShader *ps)
{
	mVertexCount = vertexCount;
	mIndexBuffer = indexBuffer;
	mIndexBufferSize = indexBufferSize;
	mVaryingNum = varyingNum;
	mPS = ps;

	for(int i = 0; i < varyingNum; i++)
	{
		mIn[i] = varyingPtr[i];
		mElementSize[i] = elementSize[i];
	}

	return 0;
}

int Rasterizer::rasterizing()
{
	// TODO: decouple framebuffer & depthbuffer with rasterizer
	mFrameBuffer = (char *)malloc(mWidth * mHeight * 4);
	mDepthBuffer = (float *)malloc(mWidth * mHeight * sizeof(float));
	for(int i = 0; i < mWidth * mHeight * 4; i += 4)
	{
		*(mFrameBuffer + i) = 0;
		*(mFrameBuffer + i + 1) = 0;
		*(mFrameBuffer + i + 2) = 0;
		*(mFrameBuffer + i + 3) = 255;
	}

	for(int i = 0; i < mWidth * mHeight; i++)
	{
		*(mDepthBuffer + i) = 1.0;
	}

	onRasterizing();

	return 0;
}

int Rasterizer::onRasterizing()
{
	return 0;
}

void ScanlineRasterizer::createGET()
{
	cout << "jzb: createGET begin " << mIndexBufferSize << endl;
	mYmin = mHeight;
	mYmax = 0;

	//TODO: clipping
	for(int i = 0; i < mIndexBufferSize; i += 3)
	{
		edge *pEdges[3];
		int ystart[3] = {0};
		int j, temp;
		float area;
		triangle *pParent;
		vec4 tmp;

		vec4 pos0 = *((vec4 *)mIn[0] + mIndexBuffer[i]);
		vec4 pos1 = *((vec4 *)mIn[0] + mIndexBuffer[i + 1]);
		vec4 pos2 = *((vec4 *)mIn[0] + mIndexBuffer[i + 2]);

		// filter out zero area triangles(2D homogeneous coordinates determinant)
		area = (pos1.x * pos2.y - pos2.x * pos1.y) - (pos0.x * pos2.y - pos2.x * pos0.y) + (pos0.x * pos1.y - pos1.x * pos0.y);
		if(EQUAL(area, 0.0))
			continue;

		// sort by Y value
		// a bit tricky here: re-write the index buffer as well
		if(pos0.y > pos1.y)
		{
			SWAP(pos0, pos1, tmp);
			SWAP(mIndexBuffer[i], mIndexBuffer[i + 1], temp);
		}
		if(pos0.y > pos2.y)
		{
			SWAP(pos0, pos2, tmp);
			SWAP(mIndexBuffer[i], mIndexBuffer[i + 2], temp);
		}
		if(pos1.y > pos2.y)
		{
			SWAP(pos1, pos2, tmp);
			SWAP(mIndexBuffer[i + 1], mIndexBuffer[i + 2], temp);
		}

		pParent = new triangle(i);
		{
			float a[9] = {pos0.x, pos0.y, 1.0, pos1.x, pos1.y, 1.0, pos2.x, pos2.y, 1.0};
			pParent->mMatrix = inverse(make_mat3(a));
			pParent->mDepths = vec3(pos0.z, pos1.z, pos2.z);
			pParent->mW = vec3(pos0.w, pos1.w, pos2.w);
			mPrimitives.push_back(pParent);
		}

		// apply top-left filling convention
		// regarding horizontal edge, just discard this edge and use the other 2 edges
		//TODO: optimize
		cout << "jzb: " << __func__ << " y0 " << pos0.y << endl;
		cout << "jzb: " << __func__ << " y1 " << pos1.y << endl;
		cout << "jzb: " << __func__ << " y2 " << pos2.y << endl;
		cout << "jzb: " << __func__ << " y0-y1 " << pos0.y -pos1.y << endl;
		if(EQUAL(pos0.y, pos1.y))
		{
			cout << "jzb: " << __func__ << " bottom tri!" << endl;
			ystart[0] = -1;
		}
		else if(EQUAL(pos1.y, pos2.y))
		{
			ystart[2] = -1;
		}

		for(j = 0; j < 3; j++)
		{
			if(ystart[j] != -1)
				pEdges[j] = new edge(pParent);
		}

		if(ystart[0] != -1)
		{
			ystart[0] = floor(pos0.y + 0.5f);
			pEdges[0]->dx = (pos1.x - pos0.x) / (pos1.y - pos0.y);
			pEdges[0]->x = pos0.x + ((ystart[0] + 0.5f) - pos0.y) * pEdges[0]->dx;
			pEdges[0]->ymax = floor(pos1.y - 0.5f);
		}

		ystart[1] = floor(pos0.y + 0.5f);
		pEdges[1]->dx = (pos2.x - pos0.x) / (pos2.y - pos0.y);
		pEdges[1]->x = pos0.x + ((ystart[1] + 0.5f) - pos0.y) * pEdges[1]->dx;
		pEdges[1]->ymax = floor(pos2.y - 0.5f);

		if(ystart[2] != -1)
		{
			ystart[2] = floor(pos1.y + 0.5f);
			pEdges[2]->dx = (pos2.x - pos1.x) / (pos2.y - pos1.y);
			pEdges[2]->x = pos1.x + ((ystart[2] + 0.5f) - pos1.y) * pEdges[2]->dx;
			pEdges[2]->ymax = floor(pos2.y - 0.5f);
		}

		mYmin = std::min((ystart[0] != -1) ? ystart[0]: ystart[1], mYmin);
		mYmax = std::max((ystart[2] != -1) ? pEdges[2]->ymax: pEdges[1]->ymax, mYmax);

		//cout << "jzb: " << __func__ << ": " << mYmin << " " << mYmax << endl;
		for(j = 0; j < 3; j++)
		{
			if(ystart[j] != -1)
			{
				if(mGET.count(ystart[j]) == 0)
					mGET.insert(pair<int, vector<edge *> >(ystart[j], vector<edge *>()));

				mGET[ystart[j]].push_back(pEdges[j]);
			}
		}
	}
	cout << "jzb: " << __func__ << ": " << mYmin << " " << mYmax << endl;
}

void ScanlineRasterizer::activateEdgesFromGET(int y)
{
	cout << "jzb: " << __func__ << ": " << y << endl;
	if(mGET.count(y))
	{
		vector<edge *> &vGET = mGET[y];
		
		cout << "jzb: " << __func__ << "GET count: " << vGET.size() << endl;
		for(vector<edge *>::iterator it = vGET.begin(); it != vGET.end(); it++)
		{
			(*it)->mParent->setActiveEdge(*it);
			mAET.push_back(*it);
		}
	}

	for(vector<edge *>::iterator it = mAET.begin(); it != mAET.end(); it++)
	{
		(*it)->bActive = true;
	}

	return;
}

// Remove unvisible edges from AET.
void ScanlineRasterizer::removeEdgeFromAET(int y)
{
	vector<edge *>::iterator it = mAET.begin();

	while(it != mAET.end())
	{
		if(y > (*it)->ymax)
		{
			(*it)->mParent->unSetActiveEdge(*it);
			it = mAET.erase(it);
		}
		else
		{
			it++;
		}
	}

	return;
}

bool ScanlineRasterizer::compareFunc(edge *pEdge1, edge *pEdge2)
{
	return (pEdge1->x <= pEdge2->x);
}

void ScanlineRasterizer::sortAETbyX()
{
	sort(mAET.begin(), mAET.end(), (static_cast<bool (*)(edge *, edge *)>(this->compareFunc)));
	return;
}

// Perspective-correct interpolation
// Vp/wp = A*(V1/w1) + B*(V2/w2) + C*(V3/w3)
// wp also needs to correct:
// 1/wp = A*(1/w1) + B*(1/w2) + C*(1/w3)
// A, B, C are the barycentric coordinates
float *ScanlineRasterizer::interpolate(vec3 &coeff, int index)
{
	float denominator = dot(coeff, vec3(1.0, 1.0, 1.0));
	size_t size = 0;

	for(int i = 1; i < mVaryingNum; i++)
	{
		size += mElementSize[i];
	}

	size *= sizeof(float);
	float *interpOutput = (float *)malloc(size);
	float *ret = interpOutput;
	for(int i = 1; i < mVaryingNum; i++)
	{
		for(int j = 0; j < mElementSize[i]; j++)
		{
			float vertAttri0 = *((float *)mIn[i] + mIndexBuffer[index] * mElementSize[i] + j);
			float vertAttri1 = *((float *)mIn[i] + mIndexBuffer[index + 1] * mElementSize[i] + j);
			float vertAttri2 = *((float *)mIn[i] + mIndexBuffer[index + 2] * mElementSize[i] + j);

			*interpOutput++ = dot(coeff, vec3(vertAttri0, vertAttri1, vertAttri2)) / denominator;
		}
	}

	return ret;
}

void ScanlineRasterizer::traversalAET(int y)
{
	vector<span> vSpans;
	
	for(vector<edge *>::iterator it = mAET.begin(); it != mAET.end(); it++)
	{
		if((*it)->bActive != true)
			continue;

		edge *pEdge = *it;
		triangle *pParent = pEdge->mParent;
		edge *pAdjcentEdge = pParent->getAdjcentEdge(pEdge);

		assert(pAdjcentEdge->bActive);

		float xleft = fmin(pEdge->x, pAdjcentEdge->x);
		float xright = fmax(pEdge->x, pAdjcentEdge->x);
		vSpans.push_back(span(xleft, xright, pParent));

		pEdge->bActive = false;
		pAdjcentEdge->bActive = false;
	}

	for(vector<span>::iterator it = vSpans.begin(); it < vSpans.end(); it++)
	{
		// top-left filling convention
		for(int x = ceil(it->xleft - 0.5f); x < ceil(it->xright - 0.5f); x++)
		{
			triangle *pParent = it->mParent;

			// There is no need to do perspective-correct for z.
			vec3 bc = pParent->calculateBC(x + 0.5f, y + 0.5f);
			float depth = dot(bc, pParent->mDepths);

			// Early-z implementation, so draw near objs first will gain more performance.
			// TODO: "defer" implementation
			if(depth < mDepthBuffer[(mHeight - y - 1) * mWidth + x])
			{
				vec3 coeff = bc * (1 / pParent->mW.x, 1 / pParent->mW.y, 1 / pParent->mW.z);
				float *psInput = interpolate(coeff, pParent->mIndex);
				mPS->attribPointer(psInput);
				mPS->setupOutputRegister(mFrameBuffer + ((mHeight - y - 1) * mWidth + x) * 4);
				mPS->execute();
				mDepthBuffer[(mHeight - y - 1) * mWidth + x] = depth;
				free(psInput);
			}
		}
	}

	return;
}

void ScanlineRasterizer::advanceEdgesInAET()
{
	for(vector<edge *>::iterator it = mAET.begin(); it < mAET.end(); it++)
	{
		(*it)->x += (*it)->dx;
	}

	return;

}

void ScanlineRasterizer::scanConversion()
{
	cout << "jzb: scan begin!" << endl;
	for(int i = mYmin; i <= mYmax; i++)
	{
		removeEdgeFromAET(i);
		activateEdgesFromGET(i);
		//sortAETbyX();
		traversalAET(i);
		advanceEdgesInAET();
	}

	cout << "jzb: scan end!" << endl;
}

void ScanlineRasterizer::finalize()
{
	mAET.clear();

	for(map<int, vector<edge *> >::iterator it = mGET.begin(); it != mGET.end(); it++)
	{
		for(vector<edge *>::iterator iter = it->second.begin(); iter < it->second.end(); iter++)
		{
			delete *iter;
		}
		it->second.clear();
	}
	mGET.clear();

	for(vector<triangle *>::iterator it = mPrimitives.begin(); it < mPrimitives.end(); it++)
		delete *it;

	mPrimitives.clear();
}

int ScanlineRasterizer::onRasterizing()
{
	cout << "jzb: onRasterizing begin" << endl;
	createGET();

	scanConversion();

	finalize();
	cout << "jzb: onRasterizing end" << endl;

	return 0;
}
