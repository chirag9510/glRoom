#include "AudioSys.h"

#include <spdlog/spdlog.h>

AudioSys::AudioSys(AudioCueSubject* audioCueSub, std::string strAssetSrc)
{
    chunkCut = Mix_LoadWAV(std::string(strAssetSrc + "audio/cut.wav").c_str());
    if (chunkCut == NULL)
    {
        spdlog::error("Failed to load cut.wav:");
        spdlog::error(Mix_GetError());
    }

    audioCueObs = std::make_unique<AudioCueObs>(audioCueSub, this);
}

AudioSys::~AudioSys()
{
    Mix_FreeChunk(chunkCut);
}
void AudioSys::PlayChunk()
{
    Mix_PlayChannel(-1, chunkCut, 0);
}