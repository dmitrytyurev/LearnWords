#pragma once

struct MandatoryCheck
{
	MandatoryCheck(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void mandatory_check(time_t freezedTime, AdditionalCheck* pAdditionalCheck, const int maxRightRepeats, const std::string& fullFileName);

	float calc_additional_word_probability(int checkByTimeWordsNumber);
	int calc_rightAnswersNum(WordsData::WordInfo& w, const int maxRightRepeats);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};
