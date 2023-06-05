//update the crt display
#pragma once
#include "Subjects.h"
#include "GeometryLoader.h"
#include <entt/entity/registry.hpp>
#include <string>

class CRTDisplaySys
{
public:
	CRTDisplaySys(entt::registry* mRegistry, std::unique_ptr<GeometryLoader>& mGeometryLoader, AudioCueSubject* audioCueSubject, std::string strAssetSrc);
	void update(const float& fDeltaTime);

private:
	void loadDisplyTexHandle(const char keyHandle, const GLuint texture);
	void updateTextureHandle(const GLuint& drawID, const GLuint64 handle);

	GLuint ssboTexHandle;															//ssbo emissive textures
	GLuint iTotalTextures;		
	entt::registry* mRegistry;
	float fCurTime, fTimer;
	std::string strDisplay;
	unsigned short charID;

	std::map<char, GLuint64> mapDisplayTexHandle;
	std::vector<GLuint> vcCRTDrawIDs;								//drawIDs of crts 

	AudioCueSubject* audioCueSubject;
};