#pragma once

namespace glsp {

struct NWMWindowInfo
{
	int width;
	int height;
	int format;
};

struct NWMBufferToDisplay
{
	void *addr;
	int   width;
	int   height;
	int   format;
};

bool glspCreateRender();
void glspSetNativeWindowInfo(NWMWindowInfo *win_info);
bool glspSwapBuffers(NWMBufferToDisplay *buf);

} // namespace glsp
