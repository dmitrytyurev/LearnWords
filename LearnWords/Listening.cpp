#include "stdafx.h"
#include "Listening.h"
#include "SoundClip.h"
#include "CommonUtility.h"
#include <vector>
#include <fstream>
#include <iostream>

//===============================================================================================
// 
//===============================================================================================

const int MAX_LINES_ON_PAGE = 20;
const int SYMBOLS_IN_ONE_LINE = 80;

struct KEY_WATCHED
{
	KEY_WATCHED() : keyCode(0), wasPressedLastTime(false) {}

	int  keyCode;
	bool wasPressedLastTime;
};

struct PAGE_INTERVAL
{
	PAGE_INTERVAL() : firstLine(0), lastLine(0) {}

	int firstLine;
	int lastLine;
};

struct SAMPLE_INTERVAL
{
	SAMPLE_INTERVAL() : startTime(0), stopTime(0) {}

	int startTime;
	int stopTime;
};


std::vector<KEY_WATCHED> keysWatched;
std::vector<std::wstring> lines;
std::vector<PAGE_INTERVAL> pageIntervals;
std::vector<SAMPLE_INTERVAL> timeSamples;

//===============================================================================================
// 
//===============================================================================================

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

//===============================================================================================
// 
//===============================================================================================

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

//===============================================================================================
// 
//===============================================================================================

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

//===============================================================================================
// 
//===============================================================================================

void load_rim_texts(const std::string& fullFileName)
{
	std::wstring fullFileNameW(fullFileName.begin(), fullFileName.end());

	FILE *file;
	std::wstring line;

	_wfopen_s(&file, fullFileNameW.c_str(), L"r,ccs=UTF-16LE");

	while (!feof(file) && !ferror(file)) 
	{
		bool isSucces = ReadOneLine(file, line);
		if (!isSucces)
			break;
		lines.push_back(line);
	}

	fclose(file);
}

//===============================================================================================
// 
//===============================================================================================

void fill_page_intervals()
{
	PAGE_INTERVAL interval;
	int linesUsed = 0;
	int firstLine = 0;

	for (int i=0; i<(int)lines.size(); ++i)
	{
		const auto& line = lines[i];

		int linesUsedByCarrentLine = 2 + line.length() / SYMBOLS_IN_ONE_LINE;
		linesUsed += linesUsedByCarrentLine;
		if (linesUsed > MAX_LINES_ON_PAGE)
		{
			linesUsed = linesUsedByCarrentLine;
			interval.firstLine = firstLine;
			interval.lastLine = i - 1;
			firstLine = i;
			pageIntervals.push_back(interval);
		}
	}
	interval.firstLine = firstLine;
	interval.lastLine = lines.size() - 1;
	pageIntervals.push_back(interval);
}

//===============================================================================================
// 
//===============================================================================================

void draw_current_texts(int selectedN)
{
	PAGE_INTERVAL workingInterval;

	for (const auto& interval : pageIntervals)
	{
		if (selectedN >= interval.firstLine && selectedN <= interval.lastLine)
		{
			workingInterval = interval;
			break;
		}
	}

	 for (int i = workingInterval.firstLine; i <= workingInterval.lastLine; ++i)
	 {
		 if (i == selectedN)
			std::wcout << L"=" << lines[i] << std::endl;
		 else
			 std::wcout << lines[i] << std::endl;
	 }
}

//===============================================================================================
// 
//===============================================================================================

void load_time_intervals(const std::string& fullFileNameStarts, const std::string& fullFileNameEnds)
{
	std::ifstream file(fullFileNameStarts);
	std::ifstream file2(fullFileNameEnds);
	if (file.is_open() && file2.is_open())
	{
		std::string strStart;
		std::string strStop;
		while (std::getline(file, strStart) && std::getline(file2, strStop))
		{
			SAMPLE_INTERVAL interval;
			interval.startTime = std::stoi(strStart);
			interval.stopTime  = std::stoi(strStop);
			timeSamples.push_back(interval);
		}
	}
}

//===============================================================================================
// 
//===============================================================================================

void listening()
{
	init_vcode_getter();
	load_rim_texts("C:/Dimka/MyLims/Eng.lim");
	fill_page_intervals();
	load_time_intervals("C:/Dimka/MyLims/SPos.lim", "C:/Dimka/MyLims/EPos.lim");
	
	std::string fullFileName = "C:\\tmp\\0.wav";
	SoundClip clip;
	int n = -1;

	while (true)
	{
		clear_console_screen();
		draw_current_texts(n == -1 ? 0 : n);

		int key = get_vcode();
		if (key == VK_ESCAPE)
		{
			getch_filtered();
			return;
		}

		if (key == VK_UP)
		{
			if (n == -1)
				n = 0;
			clip.play(fullFileName, timeSamples[n].startTime, timeSamples[n].stopTime);
		}

		if (key == VK_LEFT && n > 0)
		{
			--n;
			clip.play(fullFileName, timeSamples[n].startTime, timeSamples[n].stopTime);
		}

		if (key == VK_RIGHT && n < int(timeSamples.size()) - 1)
		{
			++n;
			clip.play(fullFileName, timeSamples[n].startTime, timeSamples[n].stopTime);
		}

		if (key == VK_DOWN)
		{
			clip.stop();
		}

//		printf("%d\n", key);
	}
	
}

