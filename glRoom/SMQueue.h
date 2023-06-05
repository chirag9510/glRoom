#pragma once
#include <queue>

enum class SMMessage
{
	NONE,
	QUIT,
	POP,
	PUSH_MAINMENU_STATE,
	PUSH_PLAY_STATE,
};

typedef std::queue<SMMessage> SMQueue;