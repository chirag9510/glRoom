#include "CRTDisplaySys.h"
#include "Components.h"
#include <spdlog/spdlog.h>

CRTDisplaySys::CRTDisplaySys(entt::registry* mRegistry, std::unique_ptr<GeometryLoader>& mGeometryLoader, AudioCueSubject* audioCueSubject, std::string strAssetSrc):
	mRegistry(mRegistry),
	fCurTime(0.f),
	fTimer(3.f),
	charID(0),
	audioCueSubject(audioCueSubject)
{
	srand(time(0));
	strDisplay = " because we seperate like ripples on a blank shore oh reckoner take me with you";

	//load all textures for the letters
	std::string strSrc = strAssetSrc + "models/textures/";
	std::string strPng = ".png";
	GLuint texture;
	for(int i = 0; i < strDisplay.length(); i++)
	{
		if (mapDisplayTexHandle.find(strDisplay[i]) == mapDisplayTexHandle.end() && strDisplay[i] != ' ')
			loadDisplyTexHandle(strDisplay[i], mGeometryLoader->loadTexture(strSrc, strDisplay[i] + strPng));
	}
	//blank texture
	loadDisplyTexHandle('0', mGeometryLoader->loadTexture(strSrc, "0.png"));

	for (auto iter = mGeometryLoader->getEntityDrawIDs().begin(); iter != mGeometryLoader->getEntityDrawIDs().end(); iter++)
	{
		if (iter->first.substr(0, 3) == "crt")
			vcCRTDrawIDs.emplace_back(iter->second);
	}

	//emissive textures 
	ssboTexHandle = mGeometryLoader->getRenderStates()[RSType::EMISSIVE].ssboFrag;
}

void CRTDisplaySys::loadDisplyTexHandle(char keyHandle, GLuint texture)
{
	GLuint64 handle = glGetTextureHandleARB(texture);
	glMakeTextureHandleResidentARB(handle);
	mapDisplayTexHandle[keyHandle] = handle;
}

void CRTDisplaySys::updateTextureHandle(const GLuint& drawID, const GLuint64 handle)
{
	GLuint64* ptrBuffer = (GLuint64*)glMapNamedBufferRange(ssboTexHandle, 0, sizeof(GLuint64) * 2 * (drawID + 1), GL_MAP_WRITE_BIT);
	ptrBuffer[drawID] = handle;
	glUnmapNamedBuffer(ssboTexHandle);
}

void CRTDisplaySys::update(const float& fDeltaTime)
{
	fCurTime += fDeltaTime;
	if (fCurTime >= fTimer)
	{
		fCurTime -= fTimer;
	
		if (strDisplay[charID] == ' ')
		{
			for (auto& drawID : vcCRTDrawIDs)
				updateTextureHandle(drawID, mapDisplayTexHandle['0']);

			fTimer = 3.f;
		}
		else if (fTimer == 3.f)
		{
			for (auto& drawID : vcCRTDrawIDs)
				updateTextureHandle(drawID, mapDisplayTexHandle[strDisplay[charID]]);
			
			fTimer = 4.f;
			audioCueSubject->notify();
		}
		else
		{
			int iRand = rand() % vcCRTDrawIDs.size();																	//4 = num monitors
			updateTextureHandle(vcCRTDrawIDs[iRand], mapDisplayTexHandle[strDisplay[charID]]);
			fTimer = 4.f;
			audioCueSubject->notify();

		}
		charID++;
		if (charID >= strDisplay.length())
			charID = 0;
	}
}