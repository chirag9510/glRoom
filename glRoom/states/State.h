#pragma once
#include "../SMQueue.h"
#include "../AppSettings.h"

#include <SDL.h>

class State
{
public:
	State(AppSettings* appSettings, SMQueue* smQueue) : appSettings(appSettings), smQueue(smQueue) {}
	virtual ~State() {}																						//virtual destructor is neccesary 
	virtual void run(SDL_Window* mWindow) = 0;

protected:
	SMQueue* smQueue;												//message ptr from state manager, enables states to communicate with each other
	AppSettings* appSettings;
};