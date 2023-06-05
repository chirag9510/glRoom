#include "PlayState.h"

#include <GL/glew.h>
#include <SDL.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>


PlayState::PlayState(AppSettings* appSettings, SMQueue* smQueue):
	State(appSettings, smQueue),
	bRBtnDown(false),
	bLBtnDown(false)
{
	mRegistry = new entt::registry();

	//subjects
	lBtnPressedSubject = new LBtnPressedSubject();
	lBtnMotionSubject = new LBtnMotionSubject();
	lBtnReleasedSubject = new LBtnReleasedSubject();
	rBtnMotionSubject = new RBtnMotionSubject();
	mBtnMotionSubject = new MBtnMotionSubject();
	mouseScrollSubject = new MouseScrollSubject();
	audioCueSubject = new AudioCueSubject();

	//maintain load order
	mCameraSys = new CameraSys(mRegistry, rBtnMotionSubject, mouseScrollSubject);
	mPhysicsSys = new PhysicsSys(mRegistry, appSettings, lBtnPressedSubject, lBtnMotionSubject, lBtnReleasedSubject);
	mInputSys = new InputSys(
		smQueue,
		mRegistry,
		mPhysicsSys->getDynamicsWorld(), 
		lBtnPressedSubject,
		lBtnMotionSubject, 
		lBtnReleasedSubject,
		rBtnMotionSubject,
		mBtnMotionSubject,
		mouseScrollSubject);

	mGeometryLoader = std::make_unique<GeometryLoader>(mRegistry, mPhysicsSys->getDynamicsWorld(), appSettings->strAssetSrc, "level.txt");
	mRenderingSys = new RenderingSys(mRegistry, mGeometryLoader, mPhysicsSys->getDynamicsWorld(), appSettings);
	mAudioSys = new AudioSys(audioCueSubject, appSettings->strAssetSrc);
	mDisplaySys = new CRTDisplaySys(mRegistry, mGeometryLoader, audioCueSubject, appSettings->strAssetSrc);
}

PlayState::~PlayState()
{
	delete lBtnPressedSubject;
	delete lBtnMotionSubject;
	delete lBtnReleasedSubject;
	delete rBtnMotionSubject;
	delete mBtnMotionSubject;
	delete mouseScrollSubject;

	delete mCameraSys;
	delete mPhysicsSys;
	delete mRenderingSys;
	delete mInputSys;
	delete mAudioSys;

	delete audioCueSubject;

	mRegistry->clear();
	delete mRegistry;
}

void PlayState::run(SDL_Window* mWindow)
{
	Uint32 uTicksLastFrame = SDL_GetTicks();
	while (true)
	{
		if (mInputSys->update())
			return;

		float fDeltaTime = static_cast<float>(SDL_GetTicks() - uTicksLastFrame) / 1000.f;
		uTicksLastFrame = SDL_GetTicks();

		//mDisplaySys->update(fDeltaTime);
		mCameraSys->update(fDeltaTime);
		mPhysicsSys->update(fDeltaTime);
		mDisplaySys->update(fDeltaTime);

		mRenderingSys->update(fDeltaTime);

		SDL_GL_SwapWindow(mWindow);
	}
}

