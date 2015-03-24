#pragma once

#include <iostream>

#define MAX_ATTRIBUTE_NUM 16

typedef enum
{
	INVALID,
	VERTEX,
	PIXEL
} ShaderType;

class Shader
{
public:
	virtual void compile() = 0;
	virtual void execute() = 0;

	Shader(ShaderType t)
	{
		type = t;
		source = NULL;
	}

	void ShaderSource(char *src)
	{
		source = src;
	}

private:
	ShaderType type;
	char *source;
};

// TODO: use shader compiler
class VertexShader: public Shader
{
public:
	virtual void compile();
	virtual void execute();
	VertexShader(): Shader(VERTEX) { mVertexCount = 0;}
	virtual int SetVertexCount(unsigned int count);
	virtual int SetAttribCount(unsigned int count);
	virtual int SetVaryingCount(unsigned int count);
	virtual int AttribPointer(int index, void *ptr);
	virtual int VaryingPointer(int index, void *ptr);

	void *mIn[MAX_ATTRIBUTE_NUM];
	void *mOut[MAX_ATTRIBUTE_NUM];
	unsigned int mVertexCount;
	unsigned int mAttribCount;
	unsigned int mVaryingCount;
};

class PixelShader: public Shader
{
public:
	PixelShader(): Shader(PIXEL) {}
	virtual void compile();
	virtual void execute();
	virtual void attribPointer(float *attri);
	virtual void setupOutputRegister(char *outReg);
	
	float *mIn;
	char *mOutReg;
};
