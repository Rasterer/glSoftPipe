#include "IAppFramework.h"

#include <cassert>

namespace glsp {

IAppFramework* IAppFramework::sAppFramework(nullptr);

void GlspApp::run()
{
	IAppFramework *app_fw = IAppFramework::open();

	if (app_fw->AttachApp(this) == false)
	{
		IAppFramework::close();
		return;
	}

	INativeWindowManager::get()->EnterLoop();
	IAppFramework::close();
}

bool GlspApp::Init()
{
	bool ret = onInit();

	if (mWidth <= 0 || mHeight <= 0 || !mName)
		ret = false;

	return ret;
}

void GlspApp::Render()
{
	onRender();
	INativeWindowManager::get()->FinishFrame();
}

IAppFramework* IAppFramework::open()
{
	if (!sAppFramework)
	{
		sAppFramework = OpenAppFramework();
	}

	return sAppFramework;
}

IAppFramework::IAppFramework():
	mNWM(nullptr),
	mApp(nullptr)
{
}

IAppFramework::~IAppFramework()
{
	if (mNWM)
	{
		mNWM->release();
		mNWM = nullptr;
	}
}

bool IAppFramework::AttachApp(GlspApp *app)
{
	assert(app);

	mApp = app;
	mNWM = INativeWindowManager::get();
	//mAppHandle = GetAppHandle(app);
	mNWM->SetAppFramework(this);

	bool ret = mNWM->InitDrawEngine();
	if (!ret)
		return false;

	// call this between InitDrawEngine and AFWCreateWindow because
	// app's onInit will call into glsp render and will set window
	// information for AFWCreateWindow.
	ret = mApp->Init();
	if (!ret)
		return false;

	return AFWCreateWindow(app->mWidth, app->mHeight, app->mName);
}

void IAppFramework::close()
{
	if (sAppFramework)
	{
		delete sAppFramework;
		sAppFramework = nullptr;
	}
}

} // namespace glsp
