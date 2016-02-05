#pragma once

#include "INativeWindowManager.h"

namespace glsp {

class GlspApp
{
public:
	friend class IAppFramework;

	GlspApp():
		mWidth(0),
		mHeight(0),
		mName(nullptr)
	{
	}
	virtual ~GlspApp() = default;

	void run();


protected:
	void setWindowInfo(int w, int h, const char *name)
	{
		mWidth = w;
		mHeight = h;
		mName = name;
	}

private:
	bool Init()
	{
		return onInit();
	}
	void Render();

	virtual bool onInit() { return true; }
	virtual void onRender() { }
	virtual void onKeyPressed(unsigned long) { }
	virtual void onMouseLeftClickDown(int, int) { }

	int mWidth;
	int mHeight;

	const char *mName;
};


class IAppFramework
{
public:
	static IAppFramework* open();
	static void close();

	IAppFramework();
	virtual ~IAppFramework();

	bool AttachApp(GlspApp *app);

	//virtual void* GetAppHandle(GlspApp *app) = 0;
	virtual void EnterLoop() = 0;
	//virtual void EventDispatch() = 0;

protected:
	void Render() { mApp->Render(); }
	void onKeyPressed(unsigned long key) { mApp->onKeyPressed(key); }
	void onMouseLeftClickDown(int x, int y) { mApp->onMouseLeftClickDown(x, y); }

	INativeWindowManager *mNWM;
	GlspApp              *mApp;
	void                 *mAppHandle;

private:
	static IAppFramework *sAppFramework;

	virtual bool AFWCreateWindow(int w, int h, const char *name) = 0;
};

IAppFramework* OpenAppFramework();

} // namespace glsp
