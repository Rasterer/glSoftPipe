#include <pthread.h>
#include "GLContext.h"

pthread_key_t TLSKey;
void initTLS()
{
	pthread_key_create(&TLSKey, NULL);
}

void deinitTLS()
{
	pthread_key_delete(TLSKey);
}

void setCurrentContext(void *gc)
{
	pthread_setspecific(TLSKey, gc);
}

GLContext * getCurrentContext()
{
	return (GLContext *)pthread_getspecific(TLSKey);
}

GLContext * CreateContext()
{
	GLContext *gc = new GLContext();
	return gc;
}

bool DestroyContext(GLContext *gc)
{
	delete gc;
	return true;
}

bool MakeCurrent(GLContext *gc)
{
	__SET_CONTEXT(gc);
	return true;
}

GLContext::GLContext()
{
}
