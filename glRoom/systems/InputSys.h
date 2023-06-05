#pragma once
#include "../SMQueue.h"
#include "Subjects.h"
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <entt/entity/registry.hpp>

class InputSys
{
public:
	InputSys(
		SMQueue* smQueue,
		entt::registry* mRegistry,
		btDiscreteDynamicsWorld* dynamicsWorld,
		LBtnPressedSubject* lBtnPressedSubject,
		LBtnMotionSubject* lBtnMotionSubject,
		LBtnReleasedSubject* lBtnReleasedSubject,
		RBtnMotionSubject* rBtnMotionSubject,
		MBtnMotionSubject* mBtnMotionSubject,
		MouseScrollSubject* mouseScrollSubject);
	~InputSys();
	
	//if returned true, then end the loop and return to statemanager to process SMQueue
	bool update();												

private:
	SMQueue* smQueue;
	entt::registry* mRegistry;
	btDiscreteDynamicsWorld* dynamicsWorld;
	bool bLBtnDown, bRBtnDown, bMBtnDown;

	entt::entity eRelMouse, eLeftMouse;

	LBtnPressedSubject* lBtnPressedSubject;
	LBtnReleasedSubject* lBtnReleasedSubject;
	LBtnMotionSubject* lBtnMotionSubject;
	RBtnMotionSubject* rBtnMotionSubject;
	MBtnMotionSubject* mBtnMotionSubject;
	MouseScrollSubject* mouseScrollSubject;
};

