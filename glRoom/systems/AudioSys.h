//only plays 1 piece
#pragma once
#include "Subjects.h"
#include "IObserver.h"

#include <SDL_mixer.h>
#include <string>

class AudioCueObs;
class AudioSys
{
public:
	AudioSys(AudioCueSubject* audioCueSub, std::string strAssetSrc);
	~AudioSys();
	void PlayChunk();

private:
	// sfx
	Mix_Chunk* chunkCut;
	std::unique_ptr<AudioCueObs> audioCueObs;
};

class AudioCueObs : public IObserver
{
public:
	AudioCueObs(AudioCueSubject* audioCueSub, AudioSys* audioSys) :
		audioSys(audioSys),
		audioCueSub(audioCueSub)
	{
		audioCueSub->attach(this);
	}
	~AudioCueObs()
	{
		audioCueSub->dettach(this);
	}

	void onNotify() override
	{
		audioSys->PlayChunk();
	}

private:
	AudioCueSubject* audioCueSub;
	AudioSys* audioSys;
};