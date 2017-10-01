#include "stdafx.h"
#include "Listening.h"
#include "SoundClip.h"
#include "CommonUtility.h"
#include <vector>
#include <fstream>
#include <iostream>


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

std::vector<std::wstring> lines;

bool ReadOneLine(FILE *File, std::wstring& Line) 
{
	wchar_t LineOfChars[512];
	wchar_t *res = fgetws(LineOfChars, 512, File);

	if (res)
	{
		Line.clear();
		Line.append(LineOfChars);
		return true;
	}
	else
		return false;
}

void load_rim_texts(const std::wstring& fullFileName) 
{
	FILE *file;
	std::wstring line;

	_wfopen_s(&file, fullFileName.c_str(), L"r,ccs=UTF-16LE");

	while (!feof(file) && !ferror(file)) 
	{
		bool isSucces = ReadOneLine(file, line);
		if (!isSucces)
			break;
		lines.push_back(line);
	}

	fclose(file);
}
void calc_first_last_lines(int selectedN, int addLinesNum, int* firstLine, int* lastLine)
{
	*firstLine = selectedN - addLinesNum;
	*firstLine = clamp_min(*firstLine, 0);
	*lastLine = *firstLine + addLinesNum*2;
	*lastLine = clamp_max(*lastLine, (int)(lines.size()) - 1);
}

const int THRESHOLD_TO_SWITCH_TO_3_LINE = 900;

void draw_current_texts(int selectedN)
{
	int firstLine = 0;
	int lastLine = 0;
	calc_first_last_lines(selectedN, 2, &firstLine, &lastLine);
	int symbolsInLines = 0;
	for (int i = firstLine; i <= lastLine; ++i)
		symbolsInLines += lines[i].length();
	 if (symbolsInLines > THRESHOLD_TO_SWITCH_TO_3_LINE)
		calc_first_last_lines(selectedN, 1, &firstLine, &lastLine);
	 for (int i = firstLine; i <= lastLine; ++i)
	 {
		 if (i == selectedN)
			std::wcout << L"===> " << lines[i] << std::endl;
		 else
			 std::wcout << lines[i] << std::endl;
	 }
}


void listening()
{
	init_vcode_getter();
	load_rim_texts(L"C:\\Dimka\\MyLims\\Eng.lim");

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
	int n = 6;

	while (true)
	{
		clear_console_screen();
		draw_current_texts(n);
		std::cout << n << std::endl;

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

		if (key == VK_RIGHT && n < int(timeSamples.size()) - 2)
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

