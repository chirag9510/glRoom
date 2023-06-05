#pragma once
#include "State.h"
#include "../systems/CameraSys.h"
#include "../systems/RenderingSys.h"
#include "../systems/PhysicsSys.h"
#include "../systems/InputSys.h"
#include "../systems/AudioSys.h"
#include "../systems/CRTDisplaySys.h"
#include "../systems/GeometryLoader.h"

#include <entt/entity/registry.hpp>
#include <memory>


class PlayState : public State
{
public:
	PlayState(AppSettings* appSettings, SMQueue* smQueue);
	~PlayState();
	void run(SDL_Window* mWindow) override;

private:

	bool bLBtnDown, bRBtnDown, bMBtnDown;
	entt::registry* mRegistry;

	RenderingSys* mRenderingSys;
	PhysicsSys* mPhysicsSys;
	CameraSys* mCameraSys;
	InputSys* mInputSys;
	CRTDisplaySys* mDisplaySys;
	AudioSys* mAudioSys;
	std::unique_ptr<GeometryLoader> mGeometryLoader;

	LBtnPressedSubject* lBtnPressedSubject;
	LBtnMotionSubject* lBtnMotionSubject;
	LBtnReleasedSubject* lBtnReleasedSubject;
	RBtnMotionSubject* rBtnMotionSubject;
	MBtnMotionSubject* mBtnMotionSubject;
	MouseScrollSubject* mouseScrollSubject;
	AudioCueSubject* audioCueSubject;
};