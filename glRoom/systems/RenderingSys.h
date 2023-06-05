//renders as well as clears/deallocates all 3d meshes
//and SCMatProj
#pragma once
#include "SystemComponents.h"
#include "Shader.h"
#include "DebugDraw.h"
#include "Subjects.h"
#include "Components.h"
#include "RenderState.h"
#include "GeometryLoader.h"
#include "../AppSettings.h"

#include <entt/entity/registry.hpp>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <glm/mat4x4.hpp>
#include <SDL_events.h>
#include <map>


class RenderingSys
{
public:
	RenderingSys(
		entt::registry* mRegistry,
		std::unique_ptr<GeometryLoader>& mGeometryLoader,
		btDiscreteDynamicsWorld* dynamicsWorld,
		AppSettings* appSettings);
	~RenderingSys();

	void update(const float& fDeltaTime);

private:
	void initFBOs();
	void updateSSBOPersMatrices();
	void updateSSBOTransforms();
	void renderScene(const float& fDeltaTime);
	void blurPass();
	float gauss(float x, float sigma2);

	std::map<RSType, RenderState> mapRenderStates;
	GLuint uboGaussWeights;	
	GLuint ssboTransforms;
	GLuint uboPerspectiveMatrices, uboFBOView;								
	GLuint drawIndirectBuffer;
	GLuint ssboFBOTransform;												//single mat4 for 2D rendering of texture
	unsigned int iTotalInstances;
	
	GeometryState geoStateBackgroundQuad;
	GeometryState geoStateStencilDraw;

	//FBOs
	GLuint fboHDR, texHDR, bufDepthStencilHDR;
	GLuint fboBlurPass, texBlurPass1, texBlurPass2;
	float fBlurBufWidth, fBlurBufHeight;
	GLuint samplerLinear, samplerNearest;
	std::vector<GLfloat> vFboTextureData;
	size_t sizeTexData;
	int mVPWidth, mVPHeight;						//default viewport size						

	//quad 
	GLuint vaoScene, vboScene, eboScene;

	AppSettings* appSettings;
	DebugDraw* debugDraw;
	RenderShader shaderRender;
	Shader shaderDebug;

	glm::mat4 matProj;

	//matView is updated by camerasys, hence why SCMatView will be needed to be accessed all the time from registry
	//for convinience this entity is kept 
	entt::entity eMatView;
	entt::entity eMatProj;
	entt::entity eDrawMode;

	entt::registry* mRegistry;
	btDiscreteDynamicsWorld* dynamicsWorld;

//	void computeMaxWhiteLum();
};
