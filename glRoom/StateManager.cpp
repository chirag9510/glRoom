 #include "StateManager.h"
#include "states/MainMenuState.h"
#include "states/PlayState.h"

#include <spdlog/spdlog.h>
#include <SDL_mixer.h>
#include <fstream>
#include <iostream>

StateManager::~StateManager()
{
	delete smQueue;
	SDL_DestroyWindow(mWindow);
	Mix_Quit();
	SDL_Quit();
}

void StateManager::run()
{
	init();

	//init main menu
	stkStates.push(std::make_unique<MainMenuState>(appSettings, smQueue, mWindow));

	while (!stkStates.empty())
	{
		stkStates.top()->run(mWindow);
		processQueue();
	}

	//write to ini file
	writeINIFile();
}

void StateManager::processQueue()
{
	while (!smQueue->empty())
	{
		switch (smQueue->front())
		{
		case SMMessage::POP:
			stkStates.pop();
			break;

		case SMMessage::PUSH_MAINMENU_STATE:
			stkStates.push(std::make_unique<MainMenuState>(appSettings, smQueue, mWindow));
			break;

		case SMMessage::PUSH_PLAY_STATE:
			stkStates.push(std::make_unique<PlayState>(appSettings, smQueue));
			break;

		case SMMessage::QUIT:
		{
			//pop every state
			while (!stkStates.empty())
				stkStates.pop();
		}
			break;
		}

		smQueue->pop();
	}
}

void StateManager::init()
{
	initAppSettings();

	//sdl glew
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		spdlog::error("SDL_mixer failed to init:");
		spdlog::error(Mix_GetError());
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	mWindow = SDL_CreateWindow("glRoom", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, appSettings->mWidth, appSettings->mHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	if (mWindow == nullptr)
		spdlog::error("SDL_CreateWindow failed");

	SDL_GLContext context = SDL_GL_CreateContext(mWindow);
	if (context == NULL)
		spdlog::error("SDL_GL_CreateContext failed");

	glewExperimental = true;
	glewInit();
	//vsync
	if (SDL_GL_SetSwapInterval(1) > 0)
		spdlog::warn("vsync disabled");

	//state message queue
	smQueue = new SMQueue();
}

void StateManager::initAppSettings()
{
	appSettings = new AppSettings();

	std::fstream fileINI;
	fileINI.open("set.ini", std::ios::in);
	if (fileINI.is_open())
	{
		std::string str;
		std::getline(fileINI, str);
		appSettings->fWindowSize = std::stof(str);
		std::getline(fileINI, str);
		appSettings->strAssetSrc = str;
		fileINI.close();

		//should have / at the end
		if (appSettings->strAssetSrc[appSettings->strAssetSrc.length() - 1] != '/')
			appSettings->strAssetSrc += "/";
	}
	else
	{
		spdlog::error("Failed to open set.ini");
		appSettings->fWindowSize = 1.f;
		appSettings->strAssetSrc = "./assets/";										//should have / at the end
	}


	appSettings->mWidth = 800 * appSettings->fWindowSize;
	appSettings->mHeight = 800 * appSettings->fWindowSize;
}

void StateManager::writeINIFile()
{
	FILE* fileINI = fopen("set.ini", "w");
	if (fileINI != nullptr)
	{
		fprintf(fileINI, "%f\n", appSettings->fWindowSize);
		fprintf(fileINI, "%s", appSettings->strAssetSrc);
		fclose(fileINI);
	}
}
