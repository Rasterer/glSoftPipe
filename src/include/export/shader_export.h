class Shader;

// For vertex shader:
// APP should use these two macros to define its own attributes(name and type)
#define VS_DECLARE_ATTRIB(attr, type)	\
	this->declareAttrib(#attr, sizeof(type));

#define VS_DEFINE_ATTRIB(attr, type)	\
	type &attr = in->defineAttrib(#attr, sizeof(type));

#define DECLARE_UNIFORM(uniform)	\
	this->declareUniform(#uniform, uniform);

// APP need implement this interface
// and pass its pointer to glShaderSource.
class ShaderFactory
{
public:
	virtual Shader *createVertexShader() = 0;
	virtual Shader *DeleteVertexShader() = 0;
	virtual Shader *createFragmentShader() = 0;
	virtual Shader *DeleteFragmentShader() = 0;
};
