#include "vs_example.h"

int vs_example::UniformMatrix4(mat4 world, mat4 view, mat4 proj)
{
	mWorld = world;
	mView = view;
	mProj = proj;
	mWVP = mProj * mView * mWorld;

	return 0;
}

void vs_example::execute(unsigned int VertexID)
{
	vec3 *const &iPos = (vec3 *)mIn[POSITION_INDEX];
	vec4 *const &iNormal = (vec4 *)mIn[NORMAL_INDEX];
	//vec2 *const &iTexCood = (vec2 *)mIn[TEXCOOD_INDEX];

	vec4 *const &oPos = (vec4 *)mOut[POSITION_INDEX];
	vec4 *const &oNormal = (vec4 *)mOut[NORMAL_INDEX];
	//vec2 *const &oTexCood = (vec2 *)mOut[TEXCOOD_INDEX];

	oPos[VertexID] = mWVP * vec4(iPos[VertexID], 1.0);
	oNormal[VertexID] = iNormal[VertexID];
	//oTexCood[VertexID] = iTexCood[VertexID];
}

void vs_example::execute()
{
	unsigned int i;
	for(i = 0; i < mVertexCount; i++)
		execute(i);
}
