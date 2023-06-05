#include "CameraSys.h"
#include "SystemComponents.h"
#include "Components.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

CameraSys::CameraSys(
	entt::registry* mRegistry,
	RBtnMotionSubject* rBtnMotionSubject,
	MouseScrollSubject* mouseScrollSubject) :
	mRegistry(mRegistry),
	fYaw(0.f),
	fSensitivity(20.f),
	fPitch(0.f),
	fRadius(45.f),
	iRelX(0),
	iRelY(0),
	iRelScrollY(0)
{
	//Observers
	relativeYawObs = std::make_unique<RelativeYawObs>(rBtnMotionSubject, this);
	relativeScrollObs = std::make_unique<RelativeScrollObs>(mouseScrollSubject, this);

	//sys components
	if (mRegistry->view<SCView>().empty())
	{
		eView = mRegistry->create();
		mRegistry->emplace<SCView>(eView);
	}
	else
		eView = mRegistry->view<SCView>()[0];

	//init camera 
	pitch(36.f);
	yaw(45.f);
}

CameraSys::~CameraSys()
{
}


void CameraSys::update(const float& fDeltaTime)
{
	if (iRelX != 0)
	{
		float fDeltaRelX = fDeltaTime * static_cast<float>(iRelX);

		yaw(fDeltaRelX * fSensitivity);

		//animate foreground texture, 0.01f sensitivity
		animateForeground(fDeltaRelX * 0.01f);

		iRelX = 0;
	}
	else if (iRelScrollY != 0)
	{
		//clamp camera zoom distance
		if ((iRelScrollY > 0 && fRadius > 10.f) || (iRelScrollY < 0 && fRadius < 150.f))
		{
			fRadius += (fDeltaTime * 55.f * static_cast<float>(-iRelScrollY));	
			pitch(0.f);
			yaw(0.f);
		}

		iRelScrollY = 0;
	}
	else if (iRelY != 0)
	{
		pitch(fDeltaTime * fSensitivity * static_cast<float>(iRelY));
		yaw(0.f);
		iRelY = 0;
	}
}

void CameraSys::genViewMatrix()
{
	mRegistry->patch<SCView>(eView,
		[this](SCView& scView)
		{
			scView.matView = glm::lookAt(vPos, glm::vec3(0.f, 5.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
			scView.vCamPos = vPos;
		}	
		);
}

void CameraSys::pitch(float p)
{
	fPitch += p;
	fPitch = fPitch > 360.f ? fPitch - 360.f : fPitch;
	vPos.y = fRadius * glm::sin(glm::radians(fPitch));
	vPos.z = fRadius * glm::cos(glm::radians(fPitch));
	genViewMatrix();
}

void CameraSys::yaw(float y)
{
	fYaw += y;
	fYaw = fYaw > 360.f ? fYaw - 360.f : fYaw;
	vPos.x = fRadius * glm::sin(glm::radians(fYaw));
	vPos.z = fRadius * glm::cos(glm::radians(fYaw));
	genViewMatrix();
}

void CameraSys::animateForeground(const float& fDeltaRelX)
{
	mRegistry->view<CBackgroundQuad>().each([fDeltaRelX](CBackgroundQuad& cQuad)
	{
		//update y coords of tex for slide effect
		float y0 = cQuad.texCoords[1] + fDeltaRelX;
		if (y0 > 0.5f)
			y0 -= 0.5f;
		else if (y0 < 0.f)
			y0 += 0.5f;


		cQuad.texCoords[1] = cQuad.texCoords[3] = y0;
		cQuad.texCoords[5] = cQuad.texCoords[7] = y0 + 0.5f;

		float* ptrBuffer = (float*)glMapNamedBufferRange(cQuad.vboTex, 0, sizeof(float) * 8, GL_MAP_WRITE_BIT);
		for (int i = 0; i < 8; i++)
			ptrBuffer[i] = cQuad.texCoords[i];
		glUnmapNamedBuffer(cQuad.vboTex);
	});
	
}

void CameraSys::setRelativeYaw(const int iRelX)
{
	this->iRelX = iRelX;
}
void CameraSys::setRelativePitch(const int iRelY)
{
	this->iRelY = iRelY;
}
void CameraSys::setRelativeRadius(const int iRelScrollY)
{
	this->iRelScrollY = iRelScrollY;
}