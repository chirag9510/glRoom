#include "GeometryLoader.h"

#include <GL/glew.h>
#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif
#include <tiny_obj_loader.h>
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include <stb_image.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <LinearMath/btDefaultMotionState.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <algorithm>

GeometryLoader::GeometryLoader(
	entt::registry* mRegistry,
	btDiscreteDynamicsWorld* dynamicsWorld,
	std::string strAssetSrc,
	std::string strLevelFile) :
	mRegistry(mRegistry),
	dynamicsWorld(dynamicsWorld),
	strAssetSrc(strAssetSrc),
	strLevelFile(strLevelFile),
	iTotalInstances(0),
	ciTotalDrawCmd(0)
{
	initGeometryInstances();
	initGeometryInstanceData();
	initGeometryBaseInstances();
	initSSBOInstanceTransforms();
	initBufferStorage();
}

GeometryLoader::~GeometryLoader()
{
	glDeleteBuffers(1, &drawIndirectBuffer);

	for (auto& tex : mapTextures)
		glDeleteTextures(1, &tex.second);
}

void GeometryLoader::initGeometryBaseInstances()
{
	//update baseInstances into entities so physics system can update transforms for instances
	mRegistry->view<CEntityType, CGeometryInstance  >().each([this](CEntityType& et, CGeometryInstance& gi)
		{
			gi.baseInstance = mapEntityBaseInstances[et.strEntityType];
		});
}

void GeometryLoader::initSSBOInstanceTransforms()
{
	glCreateBuffers(1, &ssboTransforms);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboTransforms);
	glNamedBufferStorage(ssboTransforms, sizeof(glm::mat4) * iTotalInstances, nullptr, GL_MAP_WRITE_BIT);
	glm::mat4* ptrBuffer = (glm::mat4*)glMapNamedBufferRange(ssboTransforms, 0, sizeof(glm::mat4) * iTotalInstances, GL_MAP_WRITE_BIT);
	unsigned int index = 0;
	for (auto iter = mapEntityTransforms.begin(); iter != mapEntityTransforms.end(); iter++)
	{
		for (auto matModel : iter->second)
			ptrBuffer[index++] = matModel;
	}
	glUnmapNamedBuffer(ssboTransforms);
}

void GeometryLoader::initBufferStorage()
{
	glGenBuffers(1, &drawIndirectBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawIndirectBuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand) * ciTotalDrawCmd, NULL, GL_STATIC_DRAW);

	unsigned int drawCmdOffset = 0;
	for (auto iter = mapRSLoaders.begin(); iter != mapRSLoaders.end(); iter++)
	{
		if (!iter->second.vcDrawCmd.empty())
		{
			RenderState& mapRenderState = mapRenderStates[iter->first];
			glCreateVertexArrays(1, &mapRenderState.vao);

			GLuint vbo;
			glCreateBuffers(1, &vbo);
			glNamedBufferStorage(vbo, sizeof(float) * iter->second.vertices.size(), &iter->second.vertices[0], 0);
			glVertexArrayVertexBuffer(mapRenderState.vao, 0, vbo, 0, sizeof(float) * 8);
			glVertexArrayAttribFormat(mapRenderState.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
			glVertexArrayAttribBinding(mapRenderState.vao, 0, 0);
			glEnableVertexArrayAttrib(mapRenderState.vao, 0);
			glVertexArrayAttribFormat(mapRenderState.vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
			glVertexArrayAttribBinding(mapRenderState.vao, 1, 0);
			glEnableVertexArrayAttrib(mapRenderState.vao, 1);
			glVertexArrayAttribFormat(mapRenderState.vao, 2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
			glVertexArrayAttribBinding(mapRenderState.vao, 2, 0);
			glEnableVertexArrayAttrib(mapRenderState.vao, 2);
			glDeleteBuffers(1, &vbo);

			glCreateBuffers(1, &mapRenderState.ebo);
			glNamedBufferStorage(mapRenderState.ebo, sizeof(unsigned int) * iter->second.indices.size(), &iter->second.indices[0], 0);
			glVertexArrayElementBuffer(mapRenderState.vao, mapRenderState.ebo);

			mapRenderState.drawCmdOffset = (void*)(sizeof(DrawElementsIndirectCommand) * drawCmdOffset);
			mapRenderState.primCount = iter->second.vcDrawCmd.size();

			//data mapping for the singular indirect draw buffer
			DrawElementsIndirectCommand* ptrBuffer =
				(DrawElementsIndirectCommand*)glMapBufferRange(
					GL_DRAW_INDIRECT_BUFFER,
					sizeof(DrawElementsIndirectCommand) * drawCmdOffset,
					sizeof(DrawElementsIndirectCommand) * iter->second.vcDrawCmd.size(),
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

			int index = 0;
			for (const auto& drawCmd : iter->second.vcDrawCmd)
				ptrBuffer[index++] = drawCmd;
			glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);

			drawCmdOffset += iter->second.vcDrawCmd.size();

			//bindless tex ssbo
			GLuint ssboFrag;
			if (iter->first == RSType::BASIC_KD)
			{
				glCreateBuffers(1, &ssboFrag);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboFrag);
				glNamedBufferStorage(ssboFrag, sizeof(glm::vec4) * iter->second.vcMeshKdColors.size(), &iter->second.vcMeshKdColors[0], 0);
			}
			else
			{
				std::vector<GLuint64> vcHandles;
				GLuint64 texHandle;
				vcHandles.reserve(iter->second.vcTexNames.size());
				for (auto& strTex : iter->second.vcTexNames)
				{
					texHandle = glGetTextureHandleARB(mapTextures[strTex]);
					glMakeTextureHandleResidentARB(texHandle);
					vcHandles.emplace_back(texHandle);
				}
				glCreateBuffers(1, &ssboFrag);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboFrag);
				glNamedBufferStorage(ssboFrag, sizeof(GLuint64) * vcHandles.size() * 2, &vcHandles[0], GL_MAP_WRITE_BIT);
			}
			mapRenderState.ssboFrag = ssboFrag;
		}
	}
}

void GeometryLoader::initGeometryInstanceData()
{
	for (auto iter = mapEntityTransforms.begin(); iter != mapEntityTransforms.end(); iter++)
	{
		mapEntityBaseInstances[iter->first] = iTotalInstances;

		tinyobj::ObjReaderConfig readerConfig;
		readerConfig.mtl_search_path = "./";
		tinyobj::ObjReader reader;
		if (!reader.ParseFromFile(strAssetSrc + mapEntityModelList[iter->first]))
		{
			if (!reader.Error().empty())
				spdlog::error("Reader : " + reader.Error());
		}
		if (!reader.Warning().empty())
			spdlog::warn("Warning : " + reader.Warning());

		const tinyobj::attrib_t attrib = reader.GetAttrib();
		auto& materials = reader.GetMaterials();
		auto& shapes = reader.GetShapes();

		//process textures from materials, store id to map with texname as key so the same tex isnt loaded into memory again
		for (auto& material : materials)
		{
			if (!material.diffuse_texname.empty() && mapTextures.find(material.diffuse_texname) == mapTextures.end())
				mapTextures[material.diffuse_texname] = loadTexture(strAssetSrc + "models/", material.diffuse_texname);
		}

		//load each submesh inside model
		for (size_t s = 0; s < shapes.size(); s++)
		{
			//if mesh doesnt have texture, use Kd
			RSType rsType = RSType::TEXTURED;
			if (materials[shapes[s].mesh.material_ids[0]].diffuse_texname.empty())
			{
				rsType = RSType::BASIC_KD;
				mapRSLoaders[RSType::BASIC_KD].vcMeshKdColors.emplace_back(glm::vec4(materials[shapes[s].mesh.material_ids[0]].diffuse[0], materials[shapes[s].mesh.material_ids[0]].diffuse[1], materials[shapes[s].mesh.material_ids[0]].diffuse[2], 1.f));
			}
			else
			{
				//check if mesh has emissive tex, set rendertype and use diffuse texture anyways
				if (!materials[shapes[s].mesh.material_ids[0]].emissive_texname.empty())
					rsType = RSType::EMISSIVE;

				mapRSLoaders[rsType].vcTexNames.emplace_back(materials[shapes[s].mesh.material_ids[0]].diffuse_texname);
			}

			RSLoader& rsLoader = mapRSLoaders[rsType];
			mapEntityDrawIDs[iter->first] = rsLoader.drawID++;							//update drawID

			uint32_t count = 0;
			std::unordered_map<VertexTupple, unsigned int, VertexTuppleHash> mapTupples;
			unsigned int index = 0;
			size_t iOffset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				//num vertices on each face (3 by default)
				size_t fv = shapes[s].mesh.num_face_vertices[f];

				//read each vertex attrib 
				//in the form of tupples (v/t/n) which contain element id 
				//and convert them into a single vertex index
				for (size_t v = 0; v < fv; v++)
				{
					tinyobj::index_t idx = shapes[s].mesh.indices[iOffset + v];						//index of each tupple in file 

					//check if this vertex is already loaded in buffer and given index in map
					VertexTupple tupple(idx.vertex_index, idx.texcoord_index, idx.normal_index);
					if (mapTupples.find(tupple) == mapTupples.end())
					{
						//v/n/t interleaved
						rsLoader.vertices.emplace_back(attrib.vertices[3 * size_t(idx.vertex_index)]);
						rsLoader.vertices.emplace_back(attrib.vertices[3 * size_t(idx.vertex_index) + 1]);
						rsLoader.vertices.emplace_back(attrib.vertices[3 * size_t(idx.vertex_index) + 2]);

						rsLoader.vertices.emplace_back(attrib.normals[3 * size_t(idx.normal_index)]);
						rsLoader.vertices.emplace_back(attrib.normals[3 * size_t(idx.normal_index) + 1]);
						rsLoader.vertices.emplace_back(attrib.normals[3 * size_t(idx.normal_index) + 2]);

						rsLoader.vertices.emplace_back(attrib.texcoords[2 * size_t(idx.texcoord_index)]);
						rsLoader.vertices.emplace_back(attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]);

						rsLoader.indices.emplace_back(index);
						mapTupples[tupple] = index++;
					}
					else
						rsLoader.indices.emplace_back(mapTupples[tupple]);

					count++;
				}

				iOffset += fv;
			}

			DrawElementsIndirectCommand cmd;
			cmd.count = count;
			cmd.instanceCount = iter->second.size();
			cmd.baseInstance = iTotalInstances;
			cmd.baseVertex = rsLoader.baseVertex;
			cmd.firstIndex = rsLoader.firstIndex;
			rsLoader.vcDrawCmd.emplace_back(cmd);
			ciTotalDrawCmd++;										//for the indirect buffer

			//update for next geometry
			rsLoader.baseVertex = rsLoader.vertices.size() / 8;
			rsLoader.firstIndex = rsLoader.indices.size();
		}

		//set base instance
		iTotalInstances += iter->second.size();
	}
}

void GeometryLoader::initGeometryInstances()
{
	FILE* fileLevel = fopen(std::string(strAssetSrc + strLevelFile).c_str(), "r");
	if (fileLevel != nullptr)
	{
		char szEntityType[64] = "";
		float x = 0.f, y = 0.f, z = 0.f, yaw = 0.f;
		float xDim = 0.f, yDim = 0.f, zDim = 0.f;
		while (true)
		{
			if (fscanf(fileLevel, "%s", &szEntityType) == EOF)
				break;
			fscanf(fileLevel, "%f,%f,%f", &x, &y, &z);

			if (strcmp(szEntityType, "wall") == 0)
			{
				fscanf(fileLevel, "%f,%f,%f", &xDim, &yDim, &zDim);
				createInvisibleWall(btVector3(x, y, z), btVector3(xDim, yDim, zDim));
			}
			else
			{
				fscanf(fileLevel, "%f", &yaw);
				if (strcmp(szEntityType, "book") == 0)
					createBook(btVector3(x, y, z), yaw);
				else if (strcmp(szEntityType, "desk") == 0)
					createDesk(btVector3(x, y, z), yaw);
				else if (strcmp(szEntityType, "keyboard") == 0)
					createKeyboard(btVector3(x, y, z), yaw);
				else if (strcmp(szEntityType, "shelf") == 0)
					createShelf(btVector3(x, y, z), yaw);
				else if (strcmp(szEntityType, "chair") == 0)
					createChair(btVector3(x, y, z), yaw);
				else if (strcmp(szEntityType, "monitor") == 0)
					createMonitor(btVector3(x, y, z), yaw);
				else if (strcmp(szEntityType, "moonLamp") == 0)
					createMoonLamp(btVector3(x, y, z), yaw);
				else if (strcmp(szEntityType, "plant") == 0)
					createPlantPot(btVector3(x, y, z), yaw);
				else if (strcmp(szEntityType, "bookShelf") == 0)
					createBookShelf(btVector3(x, y, z), yaw);
				else if (strcmp(szEntityType, "mug") == 0)
					createMug(btVector3(x, y, z), yaw);
				else  if (strcmp(szEntityType, "room") == 0)
					createRoom(btVector3(x, y, z), yaw);
			}
		}
	}
	else
		spdlog::error("Failed to open Level file : " + strAssetSrc + strLevelFile);
}

entt::entity GeometryLoader::createRenderableEntity(std::string strEntityType, std::string strModelPath, const CPhysicsBody cPhysicsBody, const glm::mat4 matModel)
{
	//entity type
	auto e = mRegistry->create();
	CEntityType& cEntityType = mRegistry->emplace<CEntityType>(e, strEntityType);

	//instance ID
	CGeometryInstance& cInstance = mRegistry->emplace<CGeometryInstance>(e);
	cInstance.instanceID = mapEntityTransforms[strEntityType].size();

	CPhysicsBody& physicsBody = mRegistry->emplace<CPhysicsBody>(e);
	physicsBody = cPhysicsBody;
	addRigidBody(physicsBody.rigidBody, e);

	CTransform& transform = mRegistry->emplace<CTransform>(e);
	transform.matModel = matModel;

	//Instance matModel
	mapEntityTransforms[strEntityType].emplace_back(matModel);

	//model list
	initModelList(strEntityType, strModelPath);

	return e;
}

void GeometryLoader::initModelList(std::string strEntityType, std::string strModelPath)
{
	if (mapEntityModelList.find(strEntityType) == mapEntityModelList.end())
		mapEntityModelList[strEntityType] = strModelPath;
}


void GeometryLoader::createBook(btVector3 vOrigin, float fYaw)
{
	CPhysicsBody physicsBody;
	physicsBody.collisionShape = new btBoxShape(btVector3(.715f, .1575f, 0.55f));
	btScalar fMass(.5f);
	btVector3 vLocalInertia(0.f, 0.f, 0.f);
	physicsBody.collisionShape->calculateLocalInertia(fMass, vLocalInertia);
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	btQuaternion rotation;
	rotation.setEuler(fYaw, 0.f, 0.f);
	transBody.setRotation(rotation);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	btRigidBody::btRigidBodyConstructionInfo info(fMass, physicsBody.motionState, physicsBody.collisionShape, vLocalInertia);
	physicsBody.rigidBody = new btRigidBody(info);
	physicsBody.rigidBody->setFriction(20.f);
	
	//matModel
	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	createRenderableEntity("book", "models/book.obj", physicsBody, glm::make_mat4(mat4));
}

void GeometryLoader::createDesk(btVector3 vOrigin, float fYaw)
{
	CPhysicsBody physicsBody;
	physicsBody.collisionShape = new btBoxShape(btVector3(2.f, 2.47f, 4.71f));
	btVector3 vLocalInertia(0.f, 0.f, 0.f);
	physicsBody.collisionShape->calculateLocalInertia(0.f, vLocalInertia);
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	btRigidBody::btRigidBodyConstructionInfo info(0.f, physicsBody.motionState, physicsBody.collisionShape, vLocalInertia);
	physicsBody.rigidBody = new btRigidBody(info);
	physicsBody.rigidBody->setFriction(20.f);

	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	createRenderableEntity("desk", "models/desk.obj", physicsBody, glm::make_mat4(mat4));
}

void GeometryLoader::createShelf(btVector3 vOrigin, float fYaw)
{
	CPhysicsBody physicsBody;
	physicsBody.collisionShape = createTriangleMeshShape(physicsBody, strAssetSrc + "./models/lowPoly/shelf.obj");
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	physicsBody.rigidBody = new btRigidBody(0, physicsBody.motionState, physicsBody.collisionShape);

	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	createRenderableEntity("shelf", "models/shelf.obj", physicsBody, glm::make_mat4(mat4));
}

void GeometryLoader::createKeyboard(btVector3 vOrigin, float fYaw)
{
	CPhysicsBody physicsBody;
	physicsBody.collisionShape = new btBoxShape(btVector3(0.525f, .0565f, 1.72f));
	btScalar fMass(.3f);
	btVector3 vLocalInertia(0.f, 0.f, 0.f);
	physicsBody.collisionShape->calculateLocalInertia(fMass, vLocalInertia);
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	btQuaternion rotation;
	rotation.setEuler(fYaw, 0.f, 0.f);
	transBody.setRotation(rotation);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	btRigidBody::btRigidBodyConstructionInfo info(fMass, physicsBody.motionState, physicsBody.collisionShape, vLocalInertia);
	physicsBody.rigidBody = new btRigidBody(info);
	physicsBody.rigidBody->setFriction(20.f);

	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	createRenderableEntity("keyboard", "models/keyboard.obj", physicsBody, glm::make_mat4(mat4));
}

void GeometryLoader::createPlantPot(btVector3 vOrigin, float fYaw)
{
	CPhysicsBody physicsBody;
	physicsBody.collisionShape = new btCylinderShape(btVector3(0.505f, 0.85f, .505f));
	btScalar fMass(1.5f);
	btVector3 vLocalInertia(0.f, 0.f, 0.f);
	physicsBody.collisionShape->calculateLocalInertia(fMass, vLocalInertia);
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	btRigidBody::btRigidBodyConstructionInfo info(fMass, physicsBody.motionState, physicsBody.collisionShape, vLocalInertia);
	physicsBody.rigidBody = new btRigidBody(info);
	physicsBody.rigidBody->setFriction(20.f);

	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	createRenderableEntity("plant", "models/plant.obj", physicsBody, glm::make_mat4(mat4));
}

void GeometryLoader::createBookShelf(btVector3 vOrigin, float fYaw)
{
	CPhysicsBody physicsBody;
	physicsBody.collisionShape = new btBoxShape(btVector3(.755f, .0605f, 3.015f));
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	btQuaternion rotation;
	rotation.setEuler(fYaw, 0.f, 0.f);
	transBody.setRotation(rotation);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	btRigidBody::btRigidBodyConstructionInfo info(0.f, physicsBody.motionState, physicsBody.collisionShape);
	physicsBody.rigidBody = new btRigidBody(info);
	physicsBody.rigidBody->setFriction(20.f);

	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	createRenderableEntity("bookShelf", "models/bookShelf.obj", physicsBody, glm::make_mat4(mat4));
}


void GeometryLoader::createMoonLamp(btVector3 vOrigin, float fYaw)
{
	CPhysicsBody physicsBody;
	physicsBody.collisionShape = new btSphereShape(0.505f);
	btScalar fMass(.2f);
	btVector3 vLocalInertia(0.f, 0.f, 0.f);
	physicsBody.collisionShape->calculateLocalInertia(fMass, vLocalInertia);
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	btQuaternion rotation;
	rotation.setEuler(fYaw, 0.f, 0.f);
	transBody.setRotation(rotation);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	btRigidBody::btRigidBodyConstructionInfo info(fMass, physicsBody.motionState, physicsBody.collisionShape, vLocalInertia);
	physicsBody.rigidBody = new btRigidBody(info);
	physicsBody.rigidBody->setFriction(100.f);

	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	createRenderableEntity("moonLamp", "models/moonLamp.obj", physicsBody, glm::make_mat4(mat4));
}

void GeometryLoader::createMug(btVector3 vOrigin, float fYaw)
{
	CPhysicsBody physicsBody;
	physicsBody.collisionShape = new btCylinderShape(btVector3(0.4f, 0.40f, .45f));
	btScalar fMass(0.2f);
	btVector3 vLocalInertia(0.f, 0.f, 0.f);
	physicsBody.collisionShape->calculateLocalInertia(fMass, vLocalInertia);
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	btRigidBody::btRigidBodyConstructionInfo info(fMass, physicsBody.motionState, physicsBody.collisionShape, vLocalInertia);
	physicsBody.rigidBody = new btRigidBody(info);
	physicsBody.rigidBody->setFriction(20.f);

	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	createRenderableEntity("mug", "models/mug.obj", physicsBody, glm::make_mat4(mat4));
}

void GeometryLoader::createChair(btVector3 vOrigin, float fYaw)
{
	std::string strModelPath = "models/chair.obj";
	CPhysicsBody physicsBody; 
	physicsBody.collisionShape = createConvexHullShape(strAssetSrc, strModelPath);
	btScalar fMass(30.f);
	btVector3 vLocalInertia(0.f, 0.f, 0.f);
	physicsBody.collisionShape->calculateLocalInertia(fMass, vLocalInertia);
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	btQuaternion rotation;
	rotation.setEuler(fYaw, 0.f, 0.f);
	transBody.setRotation(rotation);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	btRigidBody::btRigidBodyConstructionInfo info(fMass, physicsBody.motionState, physicsBody.collisionShape, vLocalInertia);
	physicsBody.rigidBody = new btRigidBody(info);
	physicsBody.rigidBody->setFriction(20.f);

	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	createRenderableEntity("chair", strModelPath, physicsBody, glm::make_mat4(mat4));
}

void GeometryLoader::createMonitor(btVector3 vOrigin, float fYaw)
{
	const std::string strEntityType = "crt" + std::to_string(ciCRT++);

	CPhysicsBody physicsBody;
	physicsBody.collisionShape = new btBoxShape(btVector3(1.11f, 1.11f, 1.11f));
	btScalar fMass(8.f);
	btVector3 vLocalInertia(0.f, 0.f, 0.f);
	physicsBody.collisionShape->calculateLocalInertia(fMass, vLocalInertia);
	btTransform transBody;
	transBody.setIdentity();
	transBody.setOrigin(vOrigin);
	btQuaternion rotation;
	rotation.setEuler(fYaw, 0.f, 0.f);
	transBody.setRotation(rotation);
	physicsBody.motionState = new btDefaultMotionState(transBody);
	btRigidBody::btRigidBodyConstructionInfo info(fMass, physicsBody.motionState, physicsBody.collisionShape, vLocalInertia);
	physicsBody.rigidBody = new btRigidBody(info);
	physicsBody.rigidBody->setFriction(20.f);

	btScalar mat4[16];
	transBody.getOpenGLMatrix(mat4);

	//all 4 crts act as 4 different entities but share same model
	entt::entity e = createRenderableEntity(strEntityType, "models/crt.obj", physicsBody, glm::make_mat4(mat4));

	//init emissive display for animation
	mRegistry->emplace<CCRTDisplay>(e);
}

void GeometryLoader::createRoom(btVector3 vOrigin, float fYaw)
{
	initModelList("room", "models/room.obj");
	geoStateStencilDraw = createGSStencilDraw("room");
}

void GeometryLoader::createInvisibleWall(const btVector3& vPosition, const btVector3& vDimensions)
{
	auto e = mRegistry->create();
	CPhysicsBody& physicsBody = mRegistry->emplace<CPhysicsBody>(e);

	physicsBody.collisionShape = new btBoxShape(vDimensions);
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(vPosition);
	physicsBody.motionState = new btDefaultMotionState(transform);
	btRigidBody::btRigidBodyConstructionInfo info(0.f, physicsBody.motionState, physicsBody.collisionShape, btVector3(0.f, 0.f, 0.f));
	physicsBody.rigidBody = new btRigidBody(info);
	addRigidBody(physicsBody.rigidBody, e);
}

GeometryState GeometryLoader::createGSStencilDraw(std::string strEntityType)
{
	tinyobj::ObjReaderConfig readerConfig;
	readerConfig.mtl_search_path = "./";
	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(strAssetSrc + mapEntityModelList[strEntityType]))
	{
		if (!reader.Error().empty())
			spdlog::error("Reader : " + reader.Error());
	}
	if (!reader.Warning().empty())
		spdlog::warn("Warning : " + reader.Warning());

	const tinyobj::attrib_t attrib = reader.GetAttrib();
	auto& materials = reader.GetMaterials();
	auto& shapes = reader.GetShapes();

	//load each submesh inside model
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	for (size_t s = 0; s < shapes.size(); s++)
	{
		std::unordered_map<VertexTupple, unsigned int, VertexTuppleHash> mapTupples;
		unsigned int index = 0;
		size_t iOffset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			//num vertices on each face (3 by default)
			size_t fv = shapes[s].mesh.num_face_vertices[f];

			for (size_t v = 0; v < fv; v++)
			{
				tinyobj::index_t idx = shapes[s].mesh.indices[iOffset + v];						//index of each tupple in file 

				//check if this vertex is already loaded in buffer and given index in map
				VertexTupple tupple(idx.vertex_index, idx.texcoord_index, idx.normal_index);
				if (mapTupples.find(tupple) == mapTupples.end())
				{
					//v/n/t interleaved
					vertices.emplace_back(attrib.vertices[3 * size_t(idx.vertex_index)]);
					vertices.emplace_back(attrib.vertices[3 * size_t(idx.vertex_index) + 1]);
					vertices.emplace_back(attrib.vertices[3 * size_t(idx.vertex_index) + 2]);

					vertices.emplace_back(attrib.normals[3 * size_t(idx.normal_index)]);
					vertices.emplace_back(attrib.normals[3 * size_t(idx.normal_index) + 1]);
					vertices.emplace_back(attrib.normals[3 * size_t(idx.normal_index) + 2]);

					vertices.emplace_back(attrib.texcoords[2 * size_t(idx.texcoord_index)]);
					vertices.emplace_back(attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]);

					indices.emplace_back(index);
					mapTupples[tupple] = index++;
				}
				else
					indices.emplace_back(mapTupples[tupple]);

			}

			iOffset += fv;
		}
	}

	GLuint vbo;
	GeometryState geoState;
	glCreateVertexArrays(1, &geoState.vao);
	glCreateBuffers(1, &vbo);
	glNamedBufferStorage(vbo, sizeof(float) * vertices.size(), &vertices[0], 0);
	glVertexArrayVertexBuffer(geoState.vao, 0, vbo, 0, sizeof(float) * 8);
	glVertexArrayAttribFormat(geoState.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(geoState.vao, 0, 0);
	glEnableVertexArrayAttrib(geoState.vao, 0);
	glVertexArrayAttribFormat(geoState.vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
	glVertexArrayAttribBinding(geoState.vao, 1, 0);
	glEnableVertexArrayAttrib(geoState.vao, 1);
	glVertexArrayAttribFormat(geoState.vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
	glVertexArrayAttribBinding(geoState.vao, 2, 0);
	glEnableVertexArrayAttrib(geoState.vao, 2);
	glDeleteBuffers(1, &vbo);

	glCreateBuffers(1, &geoState.ebo);
	glNamedBufferStorage(geoState.ebo, sizeof(unsigned int) * indices.size(), &indices[0], 0);
	glVertexArrayElementBuffer(geoState.vao, geoState.ebo);
	geoState.count = indices.size();
	return geoState;
}

GeometryState GeometryLoader::createGSBackgroundQuad(std::string strTexture)
{
	float vertices[] =
	{
		-1.f, 1.f, 0.f,
		1.f, 1.f, 0.f,
		1.f, -1.f, 0.f,
		-1.f, -1.f, 0.f
	};
	//0.5f y is for background texture 
	float texCoords[] =
	{
		0.f, 0.f,
		1.f, 0.f,
		1.f, 0.5f,
		0.f, 0.5f
	};
	unsigned int indices[]
	{
		0, 1, 2,
		0, 2, 3
	};

	auto e = mRegistry->create();
	auto& cQuad = mRegistry->emplace<CBackgroundQuad>(e);

	//set array tex coord
	for (int i = 0; i < 8; i++)
		cQuad.texCoords[i] = texCoords[i];

	glCreateVertexArrays(1, &geoStateBackgroundQuad.vao);
	
	GLuint vboVert;
	glCreateBuffers(1, &vboVert);
	glNamedBufferStorage(vboVert, sizeof(float) * 12, vertices, 0);
	glVertexArrayVertexBuffer(geoStateBackgroundQuad.vao, 0, vboVert, 0, sizeof(float) * 3);
	glVertexArrayAttribFormat(geoStateBackgroundQuad.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(geoStateBackgroundQuad.vao, 0, 0);
	glEnableVertexArrayAttrib(geoStateBackgroundQuad.vao, 0);
	glCreateBuffers(1, &cQuad.vboTex);
	glNamedBufferStorage(cQuad.vboTex, sizeof(float) * 8, texCoords, GL_MAP_WRITE_BIT);
	glVertexArrayVertexBuffer(geoStateBackgroundQuad.vao, 1, cQuad.vboTex, 0, sizeof(float) * 2);
	glVertexArrayAttribFormat(geoStateBackgroundQuad.vao, 2, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(geoStateBackgroundQuad.vao, 2, 1);
	glEnableVertexArrayAttrib(geoStateBackgroundQuad.vao, 2);
	glDeleteBuffers(1, &vboVert);

	glCreateBuffers(1, &geoStateBackgroundQuad.ebo);
	glNamedBufferStorage(geoStateBackgroundQuad.ebo, sizeof(unsigned int) * 6, indices, 0);
	glVertexArrayElementBuffer(geoStateBackgroundQuad.vao, geoStateBackgroundQuad.ebo);

	//this one loads png files in RGBA8 format
	GLuint64 handle[] = { glGetTextureHandleARB(loadTextureRGBA8(strAssetSrc + "models/", strTexture)) };
	glMakeTextureHandleResidentARB(handle[0]);
	glCreateBuffers(1, &geoStateBackgroundQuad.ssboFrag);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, geoStateBackgroundQuad.ssboFrag);
	glNamedBufferStorage(geoStateBackgroundQuad.ssboFrag, sizeof(GLuint64) * 2, handle, 0);

	geoStateBackgroundQuad.count = 6;
	return geoStateBackgroundQuad;
}


btTriangleMeshShape* GeometryLoader::createTriangleMeshShape(CPhysicsBody& physicsBody, std::string strSrcFile)
{
	//reads low poly version of the original rendered mesh for obvious performance reasons
	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(strSrcFile))
	{
		if (!reader.Error().empty())
			spdlog::error("Reader : " + reader.Error());
	}
	if (!reader.Warning().empty())
		spdlog::warn("Warn : " + reader.Warning());

	auto attribVertices = reader.GetAttrib().vertices;
	auto shape = reader.GetShapes()[0];

	//allocation of vertex and index data is neccessary so it isnt lost when out of the scope of this function 
	btScalar* vertices = new btScalar[attribVertices.size()];
	std::copy(attribVertices.begin(), attribVertices.end(), vertices);

	short* indices = new short[shape.mesh.indices.size()];
	unsigned short index = 0;
	for (auto& i : shape.mesh.indices)
		indices[index++] = i.vertex_index;

	btTriangleIndexVertexArray* meshInterface = new btTriangleIndexVertexArray();
	btIndexedMesh part;
	part.m_vertexBase = (const unsigned char*)vertices;
	part.m_numVertices = attribVertices.size();
	part.m_vertexStride = sizeof(btScalar) * 3;
	part.m_triangleIndexBase = (const unsigned char*)indices;
	part.m_triangleIndexStride = sizeof(short) * 3;
	part.m_numTriangles = shape.mesh.indices.size() / 3;
	part.m_indexType = PHY_SHORT;
	meshInterface->addIndexedMesh(part, PHY_SHORT);

	physicsBody.bTriangleShape = true;
	physicsBody.meshInterface = meshInterface;
	physicsBody.vertices = vertices;
	physicsBody.indices = indices;

	return new btBvhTriangleMeshShape(meshInterface, true);
}

btConvexHullShape* GeometryLoader::createConvexHullShape(std::string strSrc, std::string strFilename)
{
	auto collisionShape = new btConvexHullShape();
	collisionShape->setLocalScaling(btVector3(1.f, 1.f, 1.f));

	//check if vertex data for this mesh exists in map
	if (mapMeshConvexHulls.find(strFilename) == mapMeshConvexHulls.end())
		loadMeshConvexHullVertices(strSrc, strFilename);									//if not then load it

	for (auto& vertex : mapMeshConvexHulls[strFilename])
		collisionShape->addPoint(vertex);

	//polyhedral precision
	collisionShape->initializePolyhedralFeatures();

	return collisionShape;
}

void GeometryLoader::loadMeshConvexHullVertices(std::string strSrc, std::string strFilename)
{
	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(strSrc + strFilename))
	{
		if (!reader.Error().empty())
			spdlog::error("Reader : " + reader.Error());
	}
	if (!reader.Warning().empty())
		spdlog::warn("Warn : " + reader.Warning());

	//add to map using filename as key so the model doesnt have to be parsed again
	const tinyobj::attrib_t attrib = reader.GetAttrib();
	for (int i = 0; i < attrib.vertices.size(); i += 3)
		mapMeshConvexHulls[strFilename].emplace_back(btVector3(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]));
}

void GeometryLoader::addRigidBody(btRigidBody* rigidBody, const entt::entity e)
{
	//decativate body so it doesnt bounce around at the start
	rigidBody->setActivationState(0);

	// every rigidbody must contain in entt::entity in their userindex so its easy to work on them through entt::registry by physicsSys
	// also this func is created so as not to forget to add entity e to userIndex of rigidbody before adding to dynamicWorld
	// entt::entity is just uint32_t, hence why casting it to int
	rigidBody->setUserIndex(static_cast<int>(e));
	dynamicsWorld->addRigidBody(rigidBody);
}


GLuint GeometryLoader::loadTexture(std::string strSrc, std::string strFilename)
{
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load and generate the texture
	int width, height, nrChannels;
	//flip it 
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(std::string(strSrc + strFilename).c_str(), &width, &height, &nrChannels, STBI_rgb);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
		spdlog::error("Failed to load texture : " + strSrc + strFilename);

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

GLuint GeometryLoader::loadTextureRGBA8(std::string strSrc, std::string strFilename)
{
	if (mapTextures.find(strFilename) == mapTextures.end())
	{
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		// set the texture wrapping/filtering options (on the currently bound texture object)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// load and generate the texture
		int width, height, nrChannels;
		//flip it 
		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(std::string(strSrc + strFilename).c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
			spdlog::error("Failed to load texture : " + strSrc + strFilename);

		stbi_image_free(data);
		glBindTexture(GL_TEXTURE_2D, 0);

		//insert to map
		mapTextures[strFilename] = texture;

		return texture;
	}
	else
		return mapTextures[strFilename];
}