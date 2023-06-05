//init / deallocate system component SCMatView
#pragma once
#include "Subjects.h"
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <entt/entity/registry.hpp>
#include <glm/mat4x4.hpp>
#include <SDL_events.h>

class RelativeYawObs;
class RelativeScrollObs;

class CameraSys
{
public:
	CameraSys(entt::registry* mRegistry, RBtnMotionSubject* rBtnMotionSubject, MouseScrollSubject* mouseScrollSubject);
	~CameraSys();

	void update(const float& fDeltaTime);
	void setRelativeYaw(const int iRelX);
	void setRelativeRadius(const int iScrollY);
	void setRelativePitch(const int iRelY);
	
private:
	void genViewMatrix();
	void yaw(float yaw);
	void pitch(float pitch);
	void animateForeground(const float& fRelX);

	entt::registry* mRegistry;
	entt::entity eView;									//entity with system component SCMatView
	entt::entity eRelMouse;									//entity with system component SCRelativeMouse
	entt::entity eQuadForeground;						//foreground quad entity 

	std::unique_ptr<RelativeYawObs> relativeYawObs;
	std::unique_ptr<RelativeScrollObs> relativeScrollObs;

	glm::vec3 vPos;
 	glm::vec3 dir;
	int iRelX, iRelY, iRelScrollY;
	float fYaw, fPitch;
	float fRadius;
	float fSensitivity;
};

class RelativeYawObs : public IObserver
{
public:
	RelativeYawObs(RBtnMotionSubject* rBtnMotionSubject, CameraSys* cameraSys) :
		rBtnMotionSubject(rBtnMotionSubject),
		cameraSys(cameraSys)
	{
		rBtnMotionSubject->attach(this);
	}
	~RelativeYawObs()
	{
		rBtnMotionSubject->dettach(this);
	}
	void onNotify() override
	{
		cameraSys->setRelativeYaw(rBtnMotionSubject->getRelX());
	}

private:
	RBtnMotionSubject* rBtnMotionSubject;
	CameraSys* cameraSys;
};

class RelativeScrollObs : public IObserver
{
public:
	RelativeScrollObs(MouseScrollSubject* mouseScrollSubject, CameraSys* cameraSys) :
		mouseScrollSubject(mouseScrollSubject),
		cameraSys(cameraSys) 
	{
		mouseScrollSubject->attach(this);
	}
	~RelativeScrollObs()
	{
		mouseScrollSubject->dettach(this);
	}
	void onNotify() override
	{
		cameraSys->setRelativeRadius(mouseScrollSubject->getScrollY());
	}
private:
	MouseScrollSubject* mouseScrollSubject;
	CameraSys* cameraSys;
};