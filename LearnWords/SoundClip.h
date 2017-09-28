#pragma once
#include "Windows.h"
#include <process.h>
#include <string>
#include <thread> 
#include "HightResolutionTimer.h"

class SoundClip
{
public:
	SoundClip(const std::string& fileNameWithPath, int startMilliSec, int stopMilliSec);
	~SoundClip();

private:
	SoundClip();
	void watch_thread();
	void stop_player();

	HANDLE processHandle;
	std::thread threadObj;
	HightResolutionTimer timer;
	float prevVolume;
	int timeToTurnVolumeUp;
	int fullTimeOfPlay;
};
