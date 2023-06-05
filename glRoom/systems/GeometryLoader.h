//loads level geometry, properly arranges them for to be used by rendering system
//loads both geometries and entities independently
#pragma once
#include "RenderState.h"
#include "Components.h"

#include <string>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <entt/entity/registry.hpp>
#include <glm/mat4x4.hpp>
#include <map>

class GeometryLoader
{
public:
	GeometryLoader(
		entt::registry* mRegistry, 
		btDiscreteDynamicsWorld* dynamicsWorld, 
		std::string strAssetSrc,
		std::string strLevelFile);

	~GeometryLoader();

	GLuint getDrawIndirectBuffer() { return drawIndirectBuffer; }
	GLuint getSSBOTransforms() { return ssboTransforms; }
	GLuint getTotalInstances() { return iTotalInstances; }
	GLuint getTexture(std::string& strTex) { return mapTextures[strTex]; }
	GeometryState getGSStencilDraw() { return geoStateStencilDraw; }
	std::map<RSType, RenderState> getRenderStates() { return mapRenderStates; }
	std::map<std::string, GLuint>& getEntityDrawIDs() { return mapEntityDrawIDs; }
	GeometryState createGSBackgroundQuad(std::string strTexture);
	
	//Entity loader
	void createBook(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);
	void createDesk(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);
	void createChair(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);
	void createShelf(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);
	void createKeyboard(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);
	void createMonitor(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);
	void createBookShelf(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);
	void createMoonLamp(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);
	void createPlantPot(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);
	void createMug(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);

	void createRoom(btVector3 vOrigin = btVector3(0.f, 0.f, 0.f), float fYaw = 0.f);		
	void createInvisibleWall(const btVector3& vPosition, const btVector3& vDimensions);

	GLuint loadTexture(std::string strSrc, std::string strFilename);
	GLuint loadTextureRGBA8(std::string strSrc, std::string strFilename);

private:
	void initGeometryInstances();
	void initGeometryInstanceData();
	void initBufferStorage();
	void initGeometryBaseInstances();
	void initSSBOInstanceTransforms();
	
	void initModelList(std::string strEntityType, std::string strModelPath);
	entt::entity createRenderableEntity(std::string strEntityType, std::string strModelPath, const CPhysicsBody cPhysicsBody, const glm::mat4 matModel);				
	
	GeometryState createGSStencilDraw(std::string strEntityType);

	//gl data
	GLuint drawIndirectBuffer;																	//the single indirect draw buffer, other materials use offsets to get their appropriate data
	GLuint ssboTransforms;
	unsigned int iTotalInstances;

	std::map<RSType, RenderState> mapRenderStates;
	std::map<RSType, RSLoader> mapRSLoaders;
	std::map<std::string, std::string> mapEntityModelList;										//all paths to obj models, according to entity type
	std::map<std::string, std::vector<glm::mat4>> mapEntityTransforms;
	std::map<std::string, unsigned int> mapEntityBaseInstances;
	std::map<std::string, GLuint> mapEntityDrawIDs;													//drawIDs for all geometry, wrt to thier RenderStateType 

	GeometryState geoStateBackgroundQuad;
	GeometryState geoStateStencilDraw;

	std::string strAssetSrc;																	//asset src folder
	std::string strLevelFile;
	unsigned int ciCRT;
	unsigned int ciTotalDrawCmd;


	//use indices to create btTriangleMesh for btBvtTriangleMeshShape, provided a pointer is given
	//Model loadModel(std::string strSrc, std::string strFilename);
	btTriangleMeshShape* createTriangleMeshShape(CPhysicsBody& physicsBody, std::string strSrcFile);
	btConvexHullShape* createConvexHullShape(std::string strSrc, std::string strFilename);
	void loadMeshConvexHullVertices(std::string strSrc, std::string strFilename);
	void addRigidBody(btRigidBody* rigidBody, const entt::entity e);

	entt::registry* mRegistry;
	btDiscreteDynamicsWorld* dynamicsWorld;
	std::map<std::string, unsigned int> mapTextures;
	std::map<std::string, std::vector<btVector3>> mapMeshConvexHulls;							//keep convex hulls in a map so mesh files dont have to be read over and over again 

	
};