#include "stdafx.h"
#include "Listening.h"
#include "SoundClip.h"
#include "CommonUtility.h"
#include <vector>


struct KEY_WATCHED
{
	KEY_WATCHED() : keyCode(0), wasPressedLastTime(false) {}

	int  keyCode;
	bool wasPressedLastTime;
};

std::vector<KEY_WATCHED> keysWatched;

void init_vcode_getter()
{
	KEY_WATCHED kw;
	kw.keyCode = VK_DOWN;
	keysWatched.push_back(kw);
	kw.keyCode = VK_UP;
	keysWatched.push_back(kw);
	kw.keyCode = VK_LEFT;
	keysWatched.push_back(kw);
	kw.keyCode = VK_RIGHT;
	keysWatched.push_back(kw);
	kw.keyCode = VK_ESCAPE;
	keysWatched.push_back(kw);
}

int get_vcode()
{
	while (true)
	{
		for (auto& kw : keysWatched)
		{
			bool isPressed = GetAsyncKeyState(kw.keyCode) != 0;
			bool wasPressedLastTime = kw.wasPressedLastTime;
			kw.wasPressedLastTime = isPressed;

			if (isPressed && !wasPressedLastTime)
				return kw.keyCode;
		}
	}
}



void test()
{
	init_vcode_getter();

	std::vector<int> timeSamples;
	timeSamples.push_back(0);
	timeSamples.push_back(1210);
	timeSamples.push_back(4000);
	timeSamples.push_back(6435);
	timeSamples.push_back(9704);
	timeSamples.push_back(12855);
	timeSamples.push_back(15855);
	timeSamples.push_back(17369);

	std::string fullFileName = "C:\\tmp\\0.wav";
	SoundClip clip;
	int n = 0;

	while (true)
	{
		int key = get_vcode();
		if (key == VK_ESCAPE)
			return;

		if (key == VK_UP)
		{
			clip.play(fullFileName, timeSamples[n], timeSamples[n + 1]);
		}

		if (key == VK_LEFT && n > 0)
		{
			--n;
			clip.play(fullFileName, timeSamples[n], timeSamples[n + 1]);
		}

		if (key == VK_RIGHT && n < timeSamples.size() - 2)
		{
			++n;
			clip.play(fullFileName, timeSamples[n], timeSamples[n + 1]);
		}

		if (key == VK_DOWN)
		{
			clip.stop();
		}

//		printf("%d\n", key);
	}
	
}

