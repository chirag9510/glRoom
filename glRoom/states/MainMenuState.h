#pragma once
#include "State.h"
#include "../systems/Shader.h"
#include "../systems/AudioSys.h"
#include "../systems/Subjects.h"

#include <GL/glew.h>

class MainMenuState : public State
{
public:
	MainMenuState(AppSettings* appSettings, SMQueue* smQueue, SDL_Window* mWindow);
	~MainMenuState();
	void run(SDL_Window* mWindow) override;

private:
	AudioSys* mAudioSys;

	AudioCueSubject* audioCueSubject;

	//nuklear window properties
	float fNukWidth, fNukHeight, fNukX, fNukY;
	glm::vec3 vBkColor[3];
	unsigned short iBkColIndex;
	float fCurTime;

};