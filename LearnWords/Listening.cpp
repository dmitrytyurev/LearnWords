#include "stdafx.h"
#include "Listening.h"
#include "SoundClip.h"
#include "CommonUtility.h"
#include <vector>


void test()
{
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
	int n = 0;
	while (true)
	{
		SoundClip clip(fullFileName, timeSamples[n], timeSamples[n+1]);

		char c = getch_filtered();
		if (c == 27)
			return;

		if (c == 75  &&  n > 0) // ������� �����
			--n;

		if (c == 77  && n < timeSamples.size()-2) // ������� ������
			++n;

//		printf("%d\n", c);
	}
	
}

