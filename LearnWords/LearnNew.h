#pragma once

struct LearnWordsApp;
struct WordsData;
struct AdditionalCheck;

struct LearnNew
{
	struct WordToLearn
	{
		WordToLearn() : _index(0), _localRightAnswersNum(0) {}
		explicit WordToLearn(int index) : _index(index), _localRightAnswersNum(0) {}

		int  _index;                 // Индекс изучаемого слова в WordsOnDisk::_words
		int  _localRightAnswersNum;  // Количество непрерывных правильных ответов
	};

	LearnNew(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void learn_new(time_t freezedTime, AdditionalCheck* pAdditionalCheck);
	void learn_forgotten(time_t freezedTime, AdditionalCheck* pAdditionalCheck);
	
	void print_masked_translation(const char* _str, int symbolsToShowNum);
	void put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, bool needRandomInsert);
	bool are_all_words_learned(std::vector<WordToLearn>& queue);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};

