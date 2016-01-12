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

struct NWMCallBacks
{
	void (*GetWindowInfo)(NWMWindowInfo *win_info);
};


bool glspCreateRender(NWMCallBacks *call_backs);
bool glspSwapBuffers(NWMBufferToDisplay *buf);

} // namespace glsp
