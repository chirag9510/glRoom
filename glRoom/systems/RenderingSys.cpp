#include "RenderingSys.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <spdlog/spdlog.h>

#include <stb_image.h>

RenderingSys::RenderingSys(
	entt::registry* mRegistry,
	std::unique_ptr<GeometryLoader>& mGeometryLoader,
	btDiscreteDynamicsWorld* dynamicsWorld,
	AppSettings* appSettings) :
	mRegistry(mRegistry),
	dynamicsWorld(dynamicsWorld),
	appSettings(appSettings),
	mapRenderStates(mGeometryLoader->getRenderStates()),
	drawIndirectBuffer(mGeometryLoader->getDrawIndirectBuffer()),
	ssboTransforms(mGeometryLoader->getSSBOTransforms()),
	iTotalInstances(mGeometryLoader->getTotalInstances()),
	geoStateStencilDraw(mGeometryLoader->getGSStencilDraw()),
	shaderRender(std::string(appSettings->strAssetSrc + "shaders/render.vert").c_str(), std::string(appSettings->strAssetSrc + "shaders/render.frag").c_str()),
	shaderDebug(std::string(appSettings->strAssetSrc + "shaders/color.vert").c_str(), std::string(appSettings->strAssetSrc + "shaders/color.frag").c_str())
{
	matProj = glm::perspective(glm::radians(50.f), static_cast<float>(appSettings->mWidth) / static_cast<float>(appSettings->mHeight), 0.1f, 500.f);
	geoStateBackgroundQuad = mGeometryLoader->createGSBackgroundQuad("textures/bg.png");

	//debug draw
	debugDraw = new DebugDraw();
	dynamicsWorld->setDebugDrawer(debugDraw);
	debugDraw->setDebugMode(btIDebugDraw::DBG_DrawAabb + btIDebugDraw::DBG_DrawWireframe);

	//init system component 
	if (mRegistry->view<SCMatProjection>().empty())
	{
		eMatProj = mRegistry->create();
		mRegistry->emplace<SCMatProjection>(eMatProj, matProj);
	}
	else
		eMatProj = mRegistry->view<SCMatProjection>()[0];

	if (mRegistry->view<SCDrawMode>().empty())
	{
		eDrawMode = mRegistry->create();
		mRegistry->emplace<SCDrawMode>(eDrawMode);
	}
	else
		eDrawMode = mRegistry->view<SCDrawMode>()[0];

	if (mRegistry->view<SCView>().empty())									//get matView entity, declare if neccesary
	{
		eMatView = mRegistry->create();
		mRegistry->emplace<SCView>(eMatView, glm::lookAt(glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f)));
	}
	else
		eMatView = mRegistry->view<SCView>()[0];

	//gl rendering 
	//init ubos for view and proj matrices
	glCreateBuffers(1, &uboPerspectiveMatrices);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboPerspectiveMatrices);
	glNamedBufferStorage(uboPerspectiveMatrices, sizeof(glm::mat4) * 2, nullptr, GL_MAP_WRITE_BIT);
	glm::mat4* ptrBuffer = (glm::mat4*)glMapNamedBufferRange(uboPerspectiveMatrices, 0, sizeof(glm::mat4) * 2, GL_MAP_WRITE_BIT);
	ptrBuffer[0] = mRegistry->get<SCView>(eMatView).matView;
	ptrBuffer[1] = matProj;
	glUnmapNamedBuffer(uboPerspectiveMatrices);

	//only for FBOs that render 2D textures without depth, pairing with uboFBOView
	glm::mat4 matModel[1] = { glm::mat4(1.f) };
	glCreateBuffers(1, &ssboFBOTransform);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboFBOTransform);
	glNamedBufferStorage(ssboFBOTransform, sizeof(glm::mat4), &matModel, 0);

	//MVP matrix for FBO 2D tex rendering should be 1.f. thats how this ubo will help set it up
	glCreateBuffers(1, &uboFBOView);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboFBOView);
	glNamedBufferStorage(uboFBOView, sizeof(glm::mat4) * 2, nullptr, GL_MAP_WRITE_BIT);
	glm::mat4* ptrBufferFBO = (glm::mat4*)glMapNamedBufferRange(uboFBOView, 0, sizeof(glm::mat4) * 2, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	ptrBufferFBO[0] = glm::mat4(1.f);
	ptrBufferFBO[1] = glm::mat4(1.f);
	glUnmapNamedBuffer(uboFBOView);

	initFBOs();	
	sizeTexData = appSettings->mWidth * appSettings->mHeight;
	vFboTextureData.reserve(sizeTexData * 3);

	//other stuff
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);	
	glClearColor(0.f, 0.f, 0.f, 1.f);
	mVPWidth = appSettings->mWidth;
	mVPHeight = appSettings->mHeight;

	//default shader 
	glUseProgram(shaderRender.programID);					
}

RenderingSys::~RenderingSys()
{
	glDeleteBuffers(1, &vaoScene);
	glDeleteBuffers(1, &eboScene);

	glDeleteBuffers(1, &uboPerspectiveMatrices);
	glDeleteBuffers(1, &uboGaussWeights);
	glDeleteBuffers(1, &uboFBOView);
	glDeleteBuffers(1, &ssboFBOTransform);
	glDeleteBuffers(1, &ssboTransforms);
	glDeleteVertexArrays(1, &geoStateStencilDraw.vao);
	glDeleteVertexArrays(1, &geoStateBackgroundQuad.vao);
	glDeleteBuffers(1, &geoStateStencilDraw.ebo);
	glDeleteBuffers(1, &geoStateStencilDraw.ssboFrag);
	glDeleteBuffers(1, &geoStateBackgroundQuad.ebo);
	glDeleteBuffers(1, &geoStateBackgroundQuad.ssboFrag);

	for (auto iter = mapRenderStates.begin(); iter != mapRenderStates.end(); iter++)
	{
		glDeleteVertexArrays(1, &iter->second.vao);
		glDeleteBuffers(1, &iter->second.ebo);
		glDeleteBuffers(1, &iter->second.ssboFrag);
	}

	//clear System components
	mRegistry->clear<SCMatProjection>();

	delete debugDraw;
}

void RenderingSys::initFBOs()
{
	glUseProgram(shaderRender.programID);

	//HDR FBO
	// Create and bind the FBO
	glGenFramebuffers(1, &fboHDR);
	glBindFramebuffer(GL_FRAMEBUFFER, fboHDR);
	glGenRenderbuffers(1, &bufDepthStencilHDR);
	glBindRenderbuffer(GL_RENDERBUFFER, bufDepthStencilHDR);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, appSettings->mWidth, appSettings->mHeight);
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texHDR);
	glBindTexture(GL_TEXTURE_2D, texHDR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, appSettings->mWidth, appSettings->mHeight);										//GL_RGB will use vec3 as frag shader output at layout location 1, only rgb not rgba hence vec3
	GLenum drawBuffers[] = { GL_NONE, GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(2, drawBuffers);
	// Attach the images to the framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, bufDepthStencilHDR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texHDR, 0);
	GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (result != GL_FRAMEBUFFER_COMPLETE) 
		spdlog::error("HDR fbo is incomplete : " + std::to_string(result));

	//blur pass FBO
	fBlurBufWidth = appSettings->mWidth / 4.f;
	fBlurBufHeight = appSettings->mHeight / 4.f;
	glGenFramebuffers(1, &fboBlurPass);
	glBindFramebuffer(GL_FRAMEBUFFER, fboBlurPass);
	glGenTextures(1, &texBlurPass1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texBlurPass1);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, fBlurBufWidth, fBlurBufHeight);
	glGenTextures(1, &texBlurPass2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texBlurPass2);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, fBlurBufWidth, fBlurBufHeight);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texBlurPass1, 0);
	glDrawBuffers(2, drawBuffers);
	result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (result != GL_FRAMEBUFFER_COMPLETE)
		spdlog::error("Blur fbo is incomplete : " + std::to_string(result));


	//default
	glBindBuffer(GL_FRAMEBUFFER, 0);

	// Set up two sampler objects for linear and nearest filtering
	glGenSamplers(1, &samplerNearest);
	glGenSamplers(1, &samplerLinear);
	GLfloat border[] = { 0.0f,0.0f,0.0f,0.0f };
	glSamplerParameteri(samplerNearest, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(samplerNearest, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(samplerNearest, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(samplerNearest, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glSamplerParameterfv(samplerNearest, GL_TEXTURE_BORDER_COLOR, border);

	glSamplerParameteri(samplerLinear, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(samplerLinear, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(samplerLinear, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(samplerLinear, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glSamplerParameterfv(samplerLinear, GL_TEXTURE_BORDER_COLOR, border);

	// We want nearest sampling except for the last pass for all FBO textures, assigned from glActiveTexture(GL_TEXTUREN) for each fbo tex
	glBindSampler(0, samplerNearest);
	glBindSampler(1, samplerNearest);
	glBindSampler(2, samplerNearest);

	float weights[5], sum, sigma2 = 9;
	// Compute and sum the weights
	weights[0] = gauss(0, sigma2);
	sum = weights[0];
	for (int i = 1; i < 5; i++) 
	{
		weights[i] = gauss(float(i), sigma2);
		sum += 2 * weights[i];
	}
	for (int i = 0; i < 5; i++)
		weights[i] /= sum;

	glCreateBuffers(1, &uboGaussWeights);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, uboGaussWeights);
	glNamedBufferStorage(uboGaussWeights, sizeof(float) * 5, weights, 0);

	// scene render quad displayed on the default fbo 
	//tex coordinates differ where 0, 0 is lower left and 1, 0 is top right, otherwise the image will be rendered as inverted
	float vertices[] =
	{
		-1.f, 1.f, 0.f, 0.f, 1.f,
		1.f, 1.f, 0.f, 1.f, 1.f,
		1.f, -1.f, 0.f, 1.f, 0.f,
		-1.f, -1.f, 0.f, 0.f, 0.f
	};
	unsigned short indices[]
	{
		0, 1, 2,
		0, 2, 3
	};
	glCreateVertexArrays(1, &vaoScene);
	
	glCreateBuffers(1, &vboScene);
	glNamedBufferStorage(vboScene, sizeof(float) * 20, vertices, 0);
	glVertexArrayVertexBuffer(vaoScene, 0, vboScene, 0, sizeof(float) * 5);
	glVertexArrayAttribFormat(vaoScene, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vaoScene, 0, 0);
	glEnableVertexArrayAttrib(vaoScene, 0);
	glVertexArrayAttribFormat(vaoScene, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
	glVertexArrayAttribBinding(vaoScene, 2, 0);
	glEnableVertexArrayAttrib(vaoScene, 2);
	glDeleteBuffers(1, &vboScene);

	glCreateBuffers(1, &eboScene);
	glNamedBufferStorage(eboScene, sizeof(unsigned short) * 6, indices, 0);
	glVertexArrayElementBuffer(vaoScene, eboScene);
}

float RenderingSys::gauss(float x, float sigma2)
{
	double coeff = 1.0 / (glm::two_pi<double>() * sigma2);
	double expon = -(x * x) / (2.0 * sigma2);
	return (float)(coeff * exp(expon));
}

void RenderingSys::update(const float& fDeltaTime)
{
	//update buffer data
	updateSSBOPersMatrices();
	updateSSBOTransforms();

	renderScene(fDeltaTime);
//	computeMaxWhiteLum();		
	blurPass();

	glUniform1i(shaderRender.uniLocPass, 2);

	//pass 5 default, just display the composite quad
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, mVPWidth, mVPHeight);

	glBindSampler(1, samplerLinear);							//texBlur1 sampler linear for extra blur effect
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	glBindSampler(1, samplerNearest);							//reset to nearest
}

void RenderingSys::updateSSBOPersMatrices()
{
	glm::mat4* ptrBuffer = (glm::mat4*)glMapNamedBufferRange(uboPerspectiveMatrices, 0, sizeof(glm::mat4), GL_MAP_WRITE_BIT);
	ptrBuffer[0] = mRegistry->get<SCView>(eMatView).matView;
	glUnmapNamedBuffer(uboPerspectiveMatrices);
}

void RenderingSys::updateSSBOTransforms()
{
	glm::mat4* ptrBuffer = (glm::mat4*)glMapNamedBufferRange(ssboTransforms, 0, sizeof(glm::mat4) * iTotalInstances, GL_MAP_WRITE_BIT);
	auto view = mRegistry->view<CGeometryInstance, CTransform>();
	for (auto [e, geometryInst, transform] : view.each()) {
		if (transform.bUpdate)
		{
			ptrBuffer[geometryInst.baseInstance + geometryInst.instanceID] = transform.matModel;
			transform.bUpdate = false;
		}
	}
	glUnmapNamedBuffer(ssboTransforms);
}

void RenderingSys::renderScene(const float& fDeltaTime)
{
	//pass 1
	glUseProgram(shaderRender.programID);
	glUniform1i(shaderRender.uniLocPass, 1);
	glBindFramebuffer(GL_FRAMEBUFFER, fboHDR);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xff);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	const DrawMode drawMode = mRegistry->get<SCDrawMode>(eDrawMode).drawMode;
	if (drawMode != DrawMode::DEBUG)
	{
		//room
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glStencilFunc(GL_ALWAYS, 1, 0xff);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboPerspectiveMatrices);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboFBOTransform);
		glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shaderRender.subBasicEmissive);
		glBindVertexArray(geoStateStencilDraw.vao);
		glDrawElements(GL_TRIANGLES, geoStateStencilDraw.count, GL_UNSIGNED_INT, 0);


		//objects
		glDisable(GL_STENCIL_TEST);
		glCullFace(GL_BACK);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboTransforms);
		for (auto iter = mapRenderStates.begin(); iter != mapRenderStates.end(); iter++)
		{
			if (iter->first == RSType::BASIC_KD)	
				glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shaderRender.subBasic);			
			else if (iter->first == RSType::EMISSIVE)
			{
				glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shaderRender.subEmissive);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, iter->second.ssboFrag);
			}
			else
			{
				glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shaderRender.subTextured);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, iter->second.ssboFrag);
			}

			glBindVertexArray(iter->second.vao);
			glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, iter->second.drawCmdOffset, iter->second.primCount, 0);
		};

		
		//foreground quad
		glEnable(GL_STENCIL_TEST);
		glDisable(GL_CULL_FACE);
		glStencilFunc(GL_NOTEQUAL, 1, 0xff);
		glStencilMask(0x00);
		glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shaderRender.subEmissive);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboFBOTransform);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboFBOView);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, geoStateBackgroundQuad.ssboFrag);
		glBindVertexArray(geoStateBackgroundQuad.vao);
		glDrawElements(GL_TRIANGLES, geoStateBackgroundQuad.count, GL_UNSIGNED_INT, 0);
		glDisable(GL_STENCIL_TEST);
	}

	//debug draw
	if (drawMode != DrawMode::NORMAL)
	{
		glUseProgram(shaderDebug.programID);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboPerspectiveMatrices);
		dynamicsWorld->debugDrawWorld();

		//back to default
		glUseProgram(shaderRender.programID);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboFBOView);
	}
}


void RenderingSys::blurPass()
{
	//pass 2 , bright pass read texHDR and write to texBlur1
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, fboBlurPass);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texBlurPass1, 0);
	glViewport(0, 0, fBlurBufWidth, fBlurBufHeight);
	glClear(GL_COLOR_BUFFER_BIT);

	//reset to indexes for layout binding in shader properly
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texHDR);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texBlurPass1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texBlurPass2);
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shaderRender.subBrightPass);
	glBindVertexArray(vaoScene);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	//pass 3, read texBlur1 and write to texBlur2
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texBlurPass2, 0);
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shaderRender.subBlurVert);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	//pass 4, read texBlur2 and write to texBlur1
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texBlurPass1, 0);
	glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shaderRender.subBlurHor);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}



//void RenderingSys::computeMaxWhiteLum()
//{
//	//TODO: 
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, texHDR);
//	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, vFboTextureData.data());
//	float sum = 0.0f, maxWhiteLum = 0.f;
//	for (int i = 0; i < sizeTexData; i++) {
//		float lum = glm::dot(glm::vec3(vFboTextureData[i * 3], vFboTextureData[i * 3 + 1], vFboTextureData[i * 3 + 2]),
//			glm::vec3(0.2126f, 0.7152f, 0.0722f));
//		//max_luminance
//		maxWhiteLum = std::max(maxWhiteLum, lum);
//	}
//	glUniform1f(shaderRender.uniLocMaxWhiteLum, maxWhiteLum);
//}