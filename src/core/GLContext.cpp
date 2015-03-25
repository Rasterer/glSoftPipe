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

void *getCurrentContext()
{
	pthread_getspecific(TLSKey);
}

bool CreateContext()
{
	GLContext *gc = new GLContext();
	return true;
}

bool DestroyContext(GLContext *gc)
{
	delete gc->mBOM;
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
	mBOM = new BufferObjectMachine();
}
