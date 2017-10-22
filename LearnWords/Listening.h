#pragma once
#include <vector>

struct LearnWordsApp;
struct WordsData;

struct Listening
{
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

	Listening(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void listening(const std::string& rimFolder);

	void init_vcode_getter();
	int get_vcode();
	bool ReadOneLineW(FILE *File, std::wstring& Line);
	bool ReadOneLineA(FILE *File, std::string& Line);
	void load_rim_texts_w(const std::string& fullFileName);
	void load_rim_texts_a(const std::string& fullFileName);
	void load_rim_texts(const std::string& fullFileName);
	void fill_page_intervals();
	void draw_current_texts(int selectedN);
	void load_time_intervals(const std::string& fullFileNameStarts, const std::string& fullFileNameEnds);
	std::string get_vol_str(int vol);
	int calc_vols_num(const std::string& rimFolder);

	std::vector<KEY_WATCHED> keysWatched;
	std::vector<std::string> lines;
	std::vector<PAGE_INTERVAL> pageIntervals;
	std::vector<SAMPLE_INTERVAL> timeSamples;

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};
