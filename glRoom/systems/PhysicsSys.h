//pyBullet system, also deallocates CPhysicsBody component from all entities
#pragma once
#include "../AppSettings.h"
#include "Subjects.h"
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <BulletCollision/BroadphaseCollision/btBroadphaseInterface.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <entt/entity/registry.hpp>
#include <glm/mat4x4.hpp>
#include <SDL_events.h>

#include <btBulletDynamicsCommon.h>

class PickBodyObs;
class MoveBodyObs;
class ReleaseBodyObs;

class PhysicsSys
{
public:
	PhysicsSys(
		entt::registry* mRegistry, 
		AppSettings* appSettings,
		LBtnPressedSubject* lBtnPressedSubject,
		LBtnMotionSubject* lBtnMotionSubject,
		LBtnReleasedSubject* lBtnReleasedSubject);
	~PhysicsSys();

	void update(const float& fDeltaTime);
	btDiscreteDynamicsWorld* getDynamicsWorld();
	void pickBody(const int iMouseX, const int iMouseY);
	void moveBody(const int iMouseX, const int iMouseY);
	void releaseBody();															

private:
	//convert mouse coordinates into world position for ray 
	btVector3 getRayTo(const glm::mat4& matView, const btVector3& vRayFrom, const int& iMouseX, const int& iMouseY);

	entt::registry* mRegistry;
	btDefaultCollisionConfiguration* collisionConfig;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;

	entt::entity eMatProj;
	entt::entity eView;											//SCView system component entity

	int mWidth, mHeight;										//window dimensions

	//picked body characteristics
	btRigidBody* rigidBodyPicked;
	btTypedConstraint* constraintPicked;
	int iOriginalPickingDist;									//the distance from where the object is first picked when ray is sent from camerapos
	int iSavedActivationState;

	std::unique_ptr<PickBodyObs> pickBodyObs;
	std::unique_ptr<MoveBodyObs> moveBodyObs;
	std::unique_ptr<ReleaseBodyObs> releaseBodyObs;
};


//observer checks for left mouse click
//tell physicssys to pick a physics body
class PickBodyObs : public IObserver
{
public:
	PickBodyObs(LBtnPressedSubject* lBtnPressedSubject, PhysicsSys* physicsSys) :
		lBtnPressedSubject(lBtnPressedSubject),
		physicsSys(physicsSys)
	{
		lBtnPressedSubject->attach(this);
	}
	~PickBodyObs()
	{
		lBtnPressedSubject->dettach(this);
	}
	void onNotify() override
	{
		physicsSys->pickBody(lBtnPressedSubject->getMouseX(), lBtnPressedSubject->getMouseY());
	}

private:
	LBtnPressedSubject* lBtnPressedSubject;
	PhysicsSys* physicsSys;
};

class MoveBodyObs : public IObserver
{
public:
	MoveBodyObs(LBtnMotionSubject* lBtnMotionSubject, PhysicsSys* physicsSys) :
		lBtnMotionSubject(lBtnMotionSubject),
		physicsSys(physicsSys)
	{
		lBtnMotionSubject->attach(this);
	}
	~MoveBodyObs()
	{
		lBtnMotionSubject->dettach(this);
	}
	void onNotify() override
	{
		physicsSys->moveBody(lBtnMotionSubject->getMouseX(), lBtnMotionSubject->getMouseY());
	}
	
private:
	LBtnMotionSubject* lBtnMotionSubject;
	PhysicsSys* physicsSys;
};

class ReleaseBodyObs : public IObserver
{
public:
	ReleaseBodyObs(LBtnReleasedSubject* lBtnReleasedSubject, PhysicsSys* physicsSys) :
		lBtnReleasedSubject(lBtnReleasedSubject),
		physicsSys(physicsSys)
	{
		lBtnReleasedSubject->attach(this);
	}
	~ReleaseBodyObs()
	{
		lBtnReleasedSubject->dettach(this);
	}
	void onNotify() override
	{
		physicsSys->releaseBody();
	}

private:
	LBtnReleasedSubject* lBtnReleasedSubject;
	PhysicsSys* physicsSys;
};
