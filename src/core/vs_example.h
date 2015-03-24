#include <glm/glm.hpp>
#include "Shader.h"

using namespace glm;

class vs_example: public VertexShader
{
public:
	virtual void execute();
	void execute(unsigned int VertexID);
	vs_example(): VertexShader() {}

	int UniformMatrix4(mat4 world, mat4 view, mat4 proj);
	mat4 mWorld;
	mat4 mView;
	mat4 mProj;
	mat4 mWVP;

#define POSITION_INDEX 0
#define NORMAL_INDEX 1
#define TEXCOOD_INDEX 2
};
