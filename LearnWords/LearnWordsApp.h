#pragma once
#include <vector>
#include <ctime>
#include "WordsData.h"
#include "AdditionalCheck.h"
#include "MandatoryCheck.h"
#include "LearnNew.h"
#include "Listening.h"

struct LearnWordsApp
{
	LearnWordsApp();

	enum class RandScopePart
	{
		ALL,
		LOWER_PART,
		HI_PART,
	};

	void process(int argc, char* argv[]);

	// Вызываются классами более низкого уровня
	void clear_forgotten();
	void add_forgotten(int forgottenWordIndex);
	void get_forgotten(std::vector<int>& forgottenWordsIndices);
	bool is_quick_answer(double milliSec, const char* translation);
	void print_buttons_hints(const std::string& str, bool needRightKeyHint);
	void save();
	bool isWordJustLearnedOrForgotten(const WordsData::WordInfo& w, time_t curTime) const;
	void set_word_as_just_learned(WordsData::WordInfo& w);
	void fill_dates_and_save(WordsData::WordInfo& w, time_t currentTime, LearnWordsApp::RandScopePart randScopePart);
	void fill_rightAnswersNum(WordsData::WordInfo& w);
	void set_as_forgotten(WordsData::WordInfo& w);
	void set_as_barely_known(WordsData::WordInfo& w);
	
	// Вызываются функциями данного класса
	void fill_dates(float randDays, WordsData::WordInfo &w, time_t currentTime);
	void reset_all_words_to_repeated(int rightAnswersToSet, float minDaysRepeat, float maxDaysRepeat, time_t currentTime);
	int main_menu_choose_mode(time_t freezedTime);
	void recalc_stats(time_t curTime, int* wordsTimeToRepeatNum, int wordsByLevel[]);
	time_t get_time();
	int get_translations_num(const char* translation);

	// Поля
	WordsData _wordsOnDisk;
	std::string _fullFileName;
	std::string _fullRimPath;
	time_t _freezedTime;
	std::vector<int> _forgottenWordsIndices; // Индексы слов, которые были забыты при последней проверке слов

	AdditionalCheck _additionalCheck;
	MandatoryCheck  _mandatoryCheck;
	LearnNew        _learnNew;
	Listening       _listening;
};

