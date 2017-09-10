#pragma once

struct RepeatOfRecent
{
	RepeatOfRecent(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void repeat_of_recent(time_t freezedTime);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};
