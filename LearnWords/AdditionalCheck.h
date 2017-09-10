#pragma once
#include <vector>
#include "WordsData.h"

struct LearnWordsApp;
struct WordsData;

struct AdditionalCheck
{
	AdditionalCheck(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData), _freezedTime(0) {}
	void additional_check(time_t freezedTime, const std::string& fullFileName);

	bool can_random_tested(WordsData::WordInfo& w) const;
	int calc_max_randomTestIncID(bool isFromFastQueue);
	void fill_indices_of_random_repeat_words(std::vector<int> &indicesOfWords, bool isFromFastQueue);
	void calc_words_for_random_repeat(int* mainQueueLen, int* mainQueueSkipLoopCount, int* fastQueueLen);
	void log_random_test_words();
	int get_word_to_repeat_inner();
	int get_word_to_repeat();
	void put_word_to_end_of_random_repeat_queue_common(WordsData::WordInfo& w);
	void put_word_to_end_of_random_repeat_queue_fast(WordsData::WordInfo& w, time_t currentTime);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;

	time_t         _freezedTime;
};