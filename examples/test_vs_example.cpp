#include <iostream>
#include <string.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "vs_example.h"
#include "PrimitiveProcessor.h"
#include "ScreenMapper.h"
#include "Rasterizer.h"

using namespace glm;
extern int OffscreenFileGen(const size_t width, const size_t height, const std::string &map, const void *pixels);

int main(void)
{
	vs_example *MyShader = new vs_example();
	MyShader->compile();

	struct vsi_struct
	{
		vec3 iPos[6];
		vec4 iColor[6];
		vec2 iTexCood[3];
	} vsInput;

	struct vso_struct
	{
		vec4 oPos[18];
		vec4 oColor[18];
		vec2 oTexCood[3];
	} vsOutput;

	//vsInput.iPos[0] = vec3(-1.0f, -1.0f, 0.5773f);
	//vsInput.iPos[1] = vec3(0.0f, -1.0f, -1.15475f);
	//vsInput.iPos[2] = vec3(1.0f, -1.0f, 0.5773f);
	//vsInput.iPos[3] = vec3(0.0f, 1.0f, 0.0f);
	//vsInput.iPos[4] = vec3(-2.0, 0.5, 0.5);
	//vsInput.iPos[5] = vec3(2.0, 0.5, 0.5);

	vsInput.iPos[0] = vec3(-1.0f, -1.0f, 0.6f);
	//vsInput.iPos[1] = vec3(0.0f, -1.0f, -1.15475f);
	vsInput.iPos[1] = vec3(1.0f, -1.0f, 0.6f);
	vsInput.iPos[2] = vec3(0.0f, 1.0f, 0.0f);

	vsInput.iColor[0] = vec4(255.0, 0.0, 0.0, 255.0);
	vsInput.iColor[1] = vec4(0.0, 255.0, 0.0, 255.0);
	vsInput.iColor[2] = vec4(0.0, 0.0, 255.0, 255.0);
	//vsInput.iColor[3] = vec4(255.0, 255.0, 255.0, 255.0);
	//vsInput.iColor[4] = vec4(255.0, 255.0, 255.0, 255.0);
	//vsInput.iColor[5] = vec4(0.0, 0.0, 0.0, 255.0);

	vsInput.iTexCood[0] = vec2(0.5, 1.0);
	vsInput.iTexCood[1] = vec2(1.0, 0.0);
	vsInput.iTexCood[2] = vec2(0.0, 0.0);

	MyShader->AttribPointer(POSITION_INDEX, vsInput.iPos);
	MyShader->VaryingPointer(POSITION_INDEX, vsOutput.oPos);

	MyShader->AttribPointer(NORMAL_INDEX, vsInput.iColor);
	MyShader->VaryingPointer(NORMAL_INDEX, vsOutput.oColor);

	//MyShader->AttribPointer(TEXCOOD_INDEX, vsInput.iTexCood);
	//MyShader->VaryingPointer(TEXCOOD_INDEX, vsOutput.oTexCood);
	//mat4 myscale = scale(mat4(1.0f), vec3(2.0f, 2.0f, 2.0f));
	//mat4 myrotate = rotate(mat4(1.0f), 0.0f, vec3(0.0f, 1.0f, 0.0f));
	mat4 view = lookAt(vec3(0.0f, 0.0f, -5.0f),
					   vec3(0.0f, 0.0f, 1.0f),
					   vec3(0.0f, 1.0f, 0.0f));
	mat4 project = perspective((float)M_PI * 30.0f / 180.0f, 16.0f / 9.0f, 1.0f, 10.0f); 
	MyShader->UniformMatrix4(mat4(1.0f), view, project);
	MyShader->SetVertexCount(3);
	MyShader->execute();

	int indexBuffer[3] = {0, 2, 1};
	//int indexBuffer[12] = {0, 3, 1,
						   //1, 3, 2,
						   //2, 3, 0,
						   //0, 1, 2};
	PrimitiveProcessor *psPP = new PrimitiveProcessor();
	PrimitiveContext ctx;
	
	ctx.vertexAttri[0] = vsOutput.oPos;
	ctx.elementSize[0] = 4;
	ctx.vertexAttri[1] = vsOutput.oColor;
	ctx.elementSize[1] = 4;
	ctx.vertexCount = 3;
	ctx.attriNum = 2;
	ctx.indexBuffer = indexBuffer;
	ctx.indexBufferSize = 3;

	psPP->attachContext(&ctx);
	psPP->setupViewport(0, 0, 1280, 720);
	psPP->run();

	std::cout << "position: " << std::endl;
	std::cout << vsOutput.oPos[3].x << " " << vsOutput.oPos[3].y << " " << vsOutput.oPos[3].z << std::endl;
	std::cout << vsOutput.oPos[4].x << " " << vsOutput.oPos[4].y << " " << vsOutput.oPos[4].z << std::endl;
	std::cout << vsOutput.oPos[2].x << " " << vsOutput.oPos[2].y << " " << vsOutput.oPos[2].z << std::endl;

	Rasterizer *MyRaster = new ScanlineRasterizer();
	PixelShader *MyPS = new PixelShader();
	
	size_t elementSizeBuffer[2] = {4, 4};
	std::cout << "jzb: before raster" << std::endl;
	MyRaster->setupInput(3, 2, MyShader->mOut, elementSizeBuffer, ctx.mOutIB, ctx.mOutIBSize, MyPS);
	MyRaster->rasterizing();
	OffscreenFileGen(1280, 720, "RGBA", MyRaster->mFrameBuffer);

	return 0;
}
