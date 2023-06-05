#pragma once
#include <string>

struct AppSettings
{
	int mWidth, mHeight;		
	float fWindowSize;
	std::string strAssetSrc;										//assets folder src 
	AppSettings() : mWidth(0), mHeight(0), strAssetSrc("./assets/"), fWindowSize(1.f) {}
};