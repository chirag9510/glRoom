#include "PhysicsSys.h"
#include "Components.h"
#include "SystemComponents.h"

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>
#include <glm/gtc/type_ptr.hpp>

#include <btBulletDynamicsCommon.h>

PhysicsSys::PhysicsSys(
	entt::registry* mRegistry,
	AppSettings* appSettings,
	LBtnPressedSubject* lBtnPressedSubject,
	LBtnMotionSubject* lBtnMotionSubject,
	LBtnReleasedSubject* lBtnReleasedSubject) :
	mRegistry(mRegistry),
	mWidth(appSettings->mWidth),
	mHeight(appSettings->mHeight),
	rigidBodyPicked(nullptr),
	constraintPicked(nullptr),
	iSavedActivationState(0),
	iOriginalPickingDist(0)
{
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfig);
	overlappingPairCache = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver;

	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfig);
	dynamicsWorld->setGravity(btVector3(0, -10, 0));

	//sys components
	if (mRegistry->view<SCMatProjection>().empty())
	{
		eMatProj = mRegistry->create();
		glm::mat4 matProj = glm::perspective(glm::radians(50.f), static_cast<float>(appSettings->mWidth) / static_cast<float>(appSettings->mHeight), 0.1f, 500.f);
		mRegistry->emplace<SCMatProjection>(eMatProj, matProj);
	}
	else
		eMatProj = mRegistry->view<SCMatProjection>()[0];

	if (mRegistry->view<SCView>().empty())
	{
		eView = mRegistry->create();
		mRegistry->emplace<SCView>(eView);
	}
	else
		eView = mRegistry->view<SCView>()[0];

	//observers
	pickBodyObs = std::make_unique<PickBodyObs>(lBtnPressedSubject, this);
	moveBodyObs = std::make_unique<MoveBodyObs>(lBtnMotionSubject, this);
	releaseBodyObs = std::make_unique<ReleaseBodyObs>(lBtnReleasedSubject, this);
}

PhysicsSys::~PhysicsSys()
{
	auto view = mRegistry->view<CPhysicsBody>();
	for (auto [entity, physicsBody] : view.each())
	{
		dynamicsWorld->removeCollisionObject(physicsBody.rigidBody);
		delete physicsBody.motionState;
		delete physicsBody.collisionShape;
		delete physicsBody.rigidBody;

		if (physicsBody.bTriangleShape)
		{
			delete physicsBody.vertices;
			delete physicsBody.indices;
			delete physicsBody.meshInterface;
		}
	}
	//erase all CPhysicsBody component from all entities
	mRegistry->clear<CPhysicsBody>();

	delete dynamicsWorld;
	delete solver;
	delete overlappingPairCache;
	delete dispatcher;
	delete collisionConfig;
}

void PhysicsSys::update(const float& fDeltaTime)
{
	//update world
	dynamicsWorld->stepSimulation(fDeltaTime);
	
	btTransform worldTransform;
	btScalar mat4[16];
	mRegistry->view<CTransform, CPhysicsBody>().each([&worldTransform, &mat4](const auto entity, CTransform& transform, CPhysicsBody& physicsBody)
		{
			if (physicsBody.rigidBody->isActive())
			{
				physicsBody.motionState->getWorldTransform(worldTransform);
				worldTransform.getOpenGLMatrix(mat4);
				transform.matModel = glm::make_mat4(mat4);							//convert to glm::mat4
				transform.bUpdate = true;
			}
		});
}

void PhysicsSys::pickBody(const int iMouseX, const int iMouseY)
{
	//pick body by creating a raycast from camera pos to where the mouse clicked
	const auto& scView = mRegistry->get<SCView>(eView);
	btVector3 vRayFrom(scView.vCamPos.x, scView.vCamPos.y, scView.vCamPos.z);
	btVector3 vRayTo = getRayTo(scView.matView, vRayFrom, iMouseX, iMouseY);
	btCollisionWorld::AllHitsRayResultCallback rayCallback(vRayFrom, vRayTo);
	rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
	dynamicsWorld->rayTest(vRayFrom, vRayTo, rayCallback);

	//the ray must pass thru invisible walls, ignore it and pick up the movable object
	//check if ray hit anything
	if (rayCallback.hasHit())
	{
		//iterate every object but skip invisible walls obviously
		for (int i = 0; i < rayCallback.m_collisionObjects.size(); i++)
		{
			btRigidBody* body = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObjects[i]);
			if (body)
			{
				//other exclusions like static ground object
				if (!(body->isStaticObject() || body->isKinematicObject()))
				{
					//ray hit position on body
					btVector3 pickPos = rayCallback.m_hitPointWorld[i];

					rigidBodyPicked = body;
					//store the state, which will be restored later when the body is no longer picked
					iSavedActivationState = rigidBodyPicked->getActivationState();
					//activate body if its somehow deactive so it recieves the pybullet simulation calculation updates
					rigidBodyPicked->setActivationState(DISABLE_DEACTIVATION);

					//set pivots for constraints, mainly pivotA which is the body at its picking position
					//pivotB will be the mouse position where it goes
					btVector3 localPivot = body->getCenterOfMassTransform().inverse() * pickPos;
					btPoint2PointConstraint* p2pConstraint = new btPoint2PointConstraint(*body, localPivot);
					dynamicsWorld->addConstraint(p2pConstraint, true);
					constraintPicked = p2pConstraint;

					//mouse clamping, how responsively the object must follow the mouse
					btScalar mousePickClamping = 100.f;
					p2pConstraint->m_setting.m_impulseClamp = mousePickClamping;
					//weak constraint for picking
					p2pConstraint->m_setting.m_tau = .001f;

					mRegistry->emplace<CPickedBody>(static_cast<entt::entity>(rigidBodyPicked->getUserIndex()));

					//set the original distance from where the object was first picked
					iOriginalPickingDist = (pickPos - vRayFrom).length();

					//break and ignore the rest
					break;
				}
			}
		}
	}
}

btVector3 PhysicsSys::getRayTo(const glm::mat4& matView, const btVector3& vRayFrom, const int& iMouseX, const int& iMouseY)
{
	float fx = (2.0f * iMouseX) / (float)(mWidth)-1.0f;
	float fy = 1.0f - (2.0f * iMouseY) / (float)(mHeight);
	float z = 1.0f;
	glm::vec4 ray_eye = inverse(mRegistry->get<SCMatProjection>(eMatProj).matProj) * glm::vec4(fx, fy, -1.0, 1.0);							//to view space
	glm::vec4 ray_wor = (inverse(matView) * glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0));								
	glm::vec3 world = glm::normalize(glm::vec3(ray_wor.x, ray_wor.y, ray_wor.z));									//to world space normalized
	//set in camera direction with ray length 500 units	
	btVector3 ray = vRayFrom + btVector3(world.x, world.y, world.z) * 500.f;
	return ray;
}

void PhysicsSys::releaseBody()
{
	if (constraintPicked != nullptr)
	{
		//update registry
		mRegistry->erase<CPickedBody>(static_cast<entt::entity>(rigidBodyPicked->getUserIndex()));

		//restore state that object was on before it was picked
		rigidBodyPicked->forceActivationState(iSavedActivationState);
		rigidBodyPicked->activate();
		dynamicsWorld->removeConstraint(constraintPicked);
		delete constraintPicked;
		constraintPicked = nullptr;
		rigidBodyPicked = nullptr;
	}
}

void PhysicsSys::moveBody(const int iMouseX, const int iMouseY)
{
	if (rigidBodyPicked != nullptr && constraintPicked != nullptr)
	{
		btPoint2PointConstraint* pickCon = static_cast<btPoint2PointConstraint*>(constraintPicked);
		if (pickCon)
		{
			const auto& vCamPos = mRegistry->get<SCView>(eView).vCamPos;
			btVector3 vRayFrom(vCamPos.x, vCamPos.y, vCamPos.z);

			//keep it at the same picking distance
			btVector3 newPivotB;

			btVector3 dir = getRayTo(mRegistry->get<SCView>(eView).matView, vRayFrom, iMouseX, iMouseY) - vRayFrom;
			dir.normalize();
			dir *= iOriginalPickingDist;

			//pivotB is just following the mouse
			//connecting both pivots to move the object
			newPivotB = vRayFrom + dir;
			pickCon->setPivotB(newPivotB);
		}
	}
}

btDiscreteDynamicsWorld* PhysicsSys::getDynamicsWorld()
{
	return dynamicsWorld;
}