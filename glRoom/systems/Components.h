#pragma once
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LinearMath/btDefaultMotionState.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h>
#include <glm/mat4x4.hpp>
#include <GL/glew.h>
#include <string>

//Geometry data components
//given to entities with renderable mesh
struct CGeometryInstance
{
	GLuint baseInstance;
	GLuint instanceID;
	CGeometryInstance() : baseInstance(0), instanceID(0) {}
};

//distinguish entities by str names. makes easier to load data in buffers by identifying num instances
//also useful to get model file name and path
struct CEntityType								
{
	std::string strEntityType;
	CEntityType(std::string strEntityType) : strEntityType(strEntityType) {}
};


struct CTransform
{
	bool bUpdate;														//update ssbo data if true, done by renderingsys before drawing
	glm::mat4 matModel;								//model matrix
	CTransform(glm::mat4 matModel = glm::mat4(1.f)) : matModel(matModel), bUpdate(false)
	{
	}
};


struct CCRTDisplay
{};

//scrolling foreground
struct CBackgroundQuad
{
	GLuint vboTex;

	//update render area
	float texCoords[8];

	~CBackgroundQuad()
	{
		glDeleteBuffers(1, &vboTex);
	}
};

//btPhysics Rigid body component
struct CPhysicsBody
{
	btRigidBody* rigidBody;
	btDefaultMotionState* motionState;
	btCollisionShape* collisionShape;

	//if collision shape is btBvhTriangleMeshShape, set bTriangleShape true
	//deallocate the vertex / index / btTriangleIndexVertexArray
	bool bTriangleShape;
	btTriangleIndexVertexArray* meshInterface;
	btScalar* vertices;
	short* indices;

	CPhysicsBody() : 
		rigidBody(nullptr),
		motionState(nullptr),
		collisionShape(nullptr),
		bTriangleShape(false),
		meshInterface(nullptr),
		vertices(nullptr),
		indices(nullptr)
	{}
};

struct CPickedBody {};
