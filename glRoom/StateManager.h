#pragma once
#include "states/State.h"

#include <GL/glew.h>
#include <SDL.h>
#include <memory>
#include <stack>


class StateManager
{
public:
	~StateManager();
	void run();

private:
	void init();
	void processQueue();
	void initAppSettings();
	void writeINIFile();

	SDL_Window* mWindow;
	std::stack<std::unique_ptr<State>> stkStates;
	SMQueue* smQueue;
	AppSettings* appSettings;
};