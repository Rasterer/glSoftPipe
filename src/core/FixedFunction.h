#pragma once

// Fixed function pipeline
class FFPipeline
{
	VertexCachedAssembler	mAsbl;
	PrimitiveProcessor		mPP;
	Rasterizer				mRast;
};