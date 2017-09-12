#pragma once
#include <vector>
#include <ctime>
#include "WordsData.h"
#include "AdditionalCheck.h"
#include "MandatoryCheck.h"
#include "LearnNew.h"
#include "RepeatOfRecent.h"

struct LearnWordsApp
{
	LearnWordsApp(): _additionalCheck(this, &wordsOnDisk), _mandatoryCheck(this, &wordsOnDisk), _learnNew(this, &wordsOnDisk), _repeatOfRecent(this, &wordsOnDisk), _freezedTime(0) {}

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
	bool is_quick_answer(double milliSec);
	void print_buttons_hints(const std::string& str, bool needRightKeyHint);
	void save();
	bool isWordJustLearnedOrForgotten(const WordsData::WordInfo& w, time_t curTime) const;
	void set_word_as_just_learned(WordsData::WordInfo& w);
	void fill_dates_and_save(WordsData::WordInfo& w, time_t currentTime, LearnWordsApp::RandScopePart randScopePart);

	// Вызываются функциями данного класса
	void fill_dates(float randDays, WordsData::WordInfo &w, time_t currentTime);
	void reset_all_words_to_repeated(int rightAnswersToSet, float minDaysRepeat, float maxDaysRepeat, time_t currentTime);
	int main_menu_choose_mode();
	void recalc_stats(time_t curTime, int* wordsTimeToRepeatNum, int* wordsJustLearnedAndForgottenNum, int wordsByLevel[]);
	time_t get_time();

	// Поля
	WordsData wordsOnDisk;   // FIXME!!! все поля класса должны начинаться с _
	std::string _fullFileName;
	time_t _freezedTime;
	std::vector<int> forgottenWordsIndices; // Индексы слов, которые были забыты при последней проверке слов

	AdditionalCheck _additionalCheck;
	MandatoryCheck  _mandatoryCheck;
	LearnNew        _learnNew;
	RepeatOfRecent  _repeatOfRecent;
};

