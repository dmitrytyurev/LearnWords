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
		++n;
	}
	
}


/*
<- начать текущий фрагмент с начала если мы его играем или стоим
   если мы его играем менее 0.5с, то перескачить на фрагмент назад

-> начать следующий фрагмент (играем мы его или стоим)

!
V остановить проигрывание текущего фрагмента

*/