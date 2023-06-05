//used by systems for communication
//share single object data like projection matrix etc. to other systems
//only 1 entity should be allowed to have only 1 of these components
#pragma once
#include <glm/mat4x4.hpp>

//projection and view matrices
struct SCMatProjection
{
	glm::mat4 matProj;
	SCMatProjection(glm::mat4 matProj = glm::mat4(1.f)) : matProj(matProj) {}
};
struct SCView
{
	glm::vec3 vCamPos;																//camera position in world coords
	glm::mat4 matView;
	SCView(glm::mat4 matView = glm::mat4(1.f), glm::vec3 vCamPos = glm::vec3(1.f)) : matView(matView), vCamPos(vCamPos) {}
};

enum class DrawMode
{
	NORMAL,										//normal 3D scene without the debug lines
	NORMAL_DEBUG,								//normal 3D scene with the debug lines
	DEBUG										//only debug lines
};
struct SCDrawMode
{
	DrawMode drawMode;
	SCDrawMode(DrawMode drawMode = DrawMode::NORMAL) : drawMode(drawMode) {}
};

