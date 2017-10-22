#include "stdafx.h"
#include "Listening.h"
#include "SoundClip.h"
#include "CommonUtility.h"
#include <fstream>
#include <iostream>

//===============================================================================================
// 
//===============================================================================================

const int MAX_LINES_ON_PAGE = 30;
const int SYMBOLS_IN_ONE_LINE = 80;
const int VK_KEY_M = 77;

//===============================================================================================
// 
//===============================================================================================

void Listening::init_vcode_getter()
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
	kw.keyCode = VK_PRIOR;
	keysWatched.push_back(kw);
	kw.keyCode = VK_NEXT;
	keysWatched.push_back(kw);
	kw.keyCode = VK_KEY_M;
	keysWatched.push_back(kw);

	for (auto& kw : keysWatched)
		GetAsyncKeyState(kw.keyCode);
}

//===============================================================================================
// 
//===============================================================================================

int Listening::get_vcode()
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

bool Listening::ReadOneLineW(FILE *File, std::wstring& Line)
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

bool Listening::ReadOneLineA(FILE *File, std::string& Line)
{
	char LineOfChars[512];
	char *res = fgets(LineOfChars, 512, File);

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

void Listening::load_rim_texts_w(const std::string& fullFileName)
{
	lines.clear();
	std::wstring fullFileNameW(fullFileName.begin(), fullFileName.end());

	FILE *file;
	std::wstring linew;

	_wfopen_s(&file, fullFileNameW.c_str(), L"r,ccs=UTF-16LE");

	while (!feof(file) && !ferror(file)) 
	{
		bool isSucces = ReadOneLineW(file, linew);
		if (!isSucces)
			break;

		std::string line(linew.begin(), linew.end());
		lines.push_back(line);
	}

	fclose(file);
}

//===============================================================================================
// 
//===============================================================================================

void Listening::load_rim_texts_a(const std::string& fullFileName)
{
	lines.clear();

	FILE *file;
	std::string line;

	fopen_s(&file, fullFileName.c_str(), "r"); // , L"r,ccs=UTF-16LE");

	while (!feof(file) && !ferror(file))
	{
		bool isSucces = ReadOneLineA(file, line);
		if (!isSucces)
			break;

		lines.push_back(line);
	}

	fclose(file);
}

//===============================================================================================
// 
//===============================================================================================

void Listening::load_rim_texts(const std::string& fullFileName)
{
	std::ifstream file(fullFileName.c_str(), std::ios::in | std::ios::binary);

	if (!file.is_open())
		exit_msg("Error opening file %s\n", fullFileName.c_str());

	char ch1 = 0;
	char ch2 = 0;
	if (file.eof())
		exit_msg("Error reading file %s\n", fullFileName.c_str());

	file.get(ch1);
	file.get(ch2);
	file.close();

	if (ch1 == -1 && ch2 == -2)
		load_rim_texts_w(fullFileName);
	else
		load_rim_texts_a(fullFileName);
}

//===============================================================================================
// 
//===============================================================================================

void Listening::fill_page_intervals()
{
	pageIntervals.clear();
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

void Listening::draw_current_texts(int selectedN)
{
	PAGE_INTERVAL workingInterval;

	int i = 0;
	for (; i<(int)pageIntervals.size(); ++i)
	{
		const auto& interval = pageIntervals[i];
		if (selectedN >= interval.firstLine && selectedN <= interval.lastLine)
		{
			workingInterval = interval;
			break;
		}
	}

	std::cout << "[" << i+1 << "/" << pageIntervals.size() << "]" << std::endl;
	for (int i = workingInterval.firstLine; i <= workingInterval.lastLine; ++i)
	{
		if (i == selectedN)
		{
			std::cout << "===> " << lines[i] << std::endl;
			copy_to_clipboard(lines[i]);
		}
		else
			std::cout << lines[i] << std::endl;
	}
}

//===============================================================================================
// 
//===============================================================================================

void Listening::load_time_intervals(const std::string& fullFileNameStarts, const std::string& fullFileNameEnds)
{
	timeSamples.clear();
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

std::string Listening::get_vol_str(int vol)
{
	std::string strVol = "0000";
	
	int n = 1000;
	for (int i = 0; i < 4; ++i)
	{
		strVol[i] = vol / n + '0';
		vol -= vol / n * n;
		n /= 10;
	}

	return strVol;
}


//===============================================================================================
// 
//===============================================================================================

int Listening::calc_vols_num(const std::string& rimFolder)
{
	int volN = 1;

	while (true)
	{
		if (!if_dir_exists(rimFolder + get_vol_str(volN)))
			break;
		++volN;
	}

	return volN - 1;
}

//===============================================================================================
// 
//===============================================================================================

void Listening::listening(const std::string& rimFolder)
{
	init_vcode_getter();
	int volCurrent = 1;
	int volTotal = calc_vols_num(rimFolder);
	bool startsNow = false;

	while (true)
	{
		load_rim_texts(rimFolder + get_vol_str(volCurrent) + "\\Eng.lim");
		fill_page_intervals();
		load_time_intervals(rimFolder + get_vol_str(volCurrent) + "\\SPos.lim", rimFolder + get_vol_str(volCurrent) + "\\EPos.lim");

		std::string fullFileName = rimFolder + get_vol_str(volCurrent) + "\\0.wav";
		SoundClip clip;
		int n = -1;

		while (true)
		{
			clear_console_screen();
			std::cout << "[" << volCurrent << "\\" << volTotal << "]" << std::endl;
			draw_current_texts(n == -1 ? 0 : n);

			int key = 0;
			if (startsNow)
			{
				startsNow = false;
				goto m3;
			}
			key = get_vcode();
			if (key == VK_ESCAPE)
			{
				getch_filtered();
				return;
			}

			if (key == VK_RIGHT)
			{
				if (n == -1)
					n = 0;
				clip.play(fullFileName, timeSamples[n].startTime, timeSamples[n].stopTime);
			}

			if (key == VK_UP)
			{
				if (n > 0)
				{
					--n;
					clip.play(fullFileName, timeSamples[n].startTime, timeSamples[n].stopTime);
				}
				else
					goto m2;
			}

			if (key == VK_DOWN)
			{
m3:				if (n < int(timeSamples.size()) - 1)
				{
					++n;
					clip.play(fullFileName, timeSamples[n].startTime, timeSamples[n].stopTime);
				}
				else
				{
					startsNow = true;
					goto m1;
				}
			}

			if (key == VK_LEFT)
			{
				clip.stop();
			}

			if (key == VK_PRIOR)
			{
m2:				if (volCurrent > 1)
				{
					--volCurrent;
					break;
				}
			}

			if (key == VK_NEXT)
			{
				startsNow = false;
m1:				if (volCurrent < volTotal)
				{
					++volCurrent;
					break;
				}
			}

			if (key == VK_KEY_M)
			{
			}
		}
	}
}

