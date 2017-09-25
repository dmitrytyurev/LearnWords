#pragma once
#include "Windows.h"
#include <process.h>
#include <string>

class SoundClip
{
public:
	SoundClip(const std::string& fileNameWithPath, int startSec, int stopSec);
	~SoundClip();

private:

	SoundClip();

	HANDLE hdl;
};
