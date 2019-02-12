#pragma once
#include <vector>
#include <set>
#include <ctime>
#include "WordsData.h"
#include "AdditionalCheck.h"
#include "MandatoryCheck.h"
#include "LearnNew.h"

struct WordToCheck
{
	WordToCheck() : _index(0), _sortCoeff(0) {}
	WordToCheck(int index) : _index(index), _sortCoeff(0) {}

	int   _index;       // ������ ������������ ����� � WordsOnDisk::_words
	float _sortCoeff;   // ������������� �����������, ���� ���� ��������� ������ ����, ��� �� ����� ������ ���������
};

struct WordRememberedLong  // �����, ������� ������� �������� ���������, �� ��������� �����
{
	WordRememberedLong(int wordIndex, double wordDuration) : index(wordIndex), durationOfRemember(wordDuration) {}
	bool operator<(const WordRememberedLong& r) const {
		return index < r.index;
	}
	bool operator==(const WordRememberedLong& r) const {
		return index == r.index;
	}

	int index = 0;                 // ������ ����� � _wordsOnDisk
	double durationOfRemember = 0; // ������������ ����������� �����
};

struct LearnWordsApp
{
	LearnWordsApp();

	void process(int argc, char* argv[]);

	// ���������� �������� ����� ������� ������
	void clear_forgotten();
	void add_forgotten(int forgottenWordIndex);
	void get_forgotten(std::vector<int>& forgottenWordsIndices);
	bool is_in_forgotten(int wordIndex);
	void erase_from_remembered_long(int wordIndex);
	void update_time_remembered_long(int wordIndex, double durationOfRemember);

	bool is_quick_answer(double milliSec, const char* translation, bool* ifTooLongAnswer = nullptr, double* extraDurationForAnswer = nullptr);
	void print_buttons_hints(const std::string& str, bool needRightKeyHint);
	void save();
	bool isWordJustLearnedOrForgotten(const WordsData::WordInfo& w, time_t curTime) const;
	void set_word_as_just_learned(WordsData::WordInfo& w);
	void fill_dates_and_save(WordsData::WordInfo& w, time_t currentTime, bool needAdvance_RightAnswersNum, bool isQuickAnswer);
	void set_as_forgotten(WordsData::WordInfo& w);
	void set_as_barely_known(WordsData::WordInfo& w);
	void collect_words_to_mandatory_check(std::vector<WordToCheck>& wordsToRepeat, time_t freezedTime);
	
	// ���������� ��������� ������� ������
	void fill_dates(float randDays, WordsData::WordInfo &w, time_t currentTime);
	void reset_all_words_to_repeated(int rightAnswersToSet, float minDaysRepeat, float maxDaysRepeat, time_t currentTime);
	int main_menu_choose_mode(time_t freezedTime);
	void recalc_stats(time_t curTime, int* wordsTimeToRepeatNum, int wordsByLevel[]);
	time_t get_time();
	int get_translations_num(const char* translation);

	// ����
	WordsData _wordsOnDisk;
	std::string _fullFileName;
	std::string _fullRimPath;
	time_t _freezedTime;
	std::vector<int> _forgottenWordsIndices; // ������� ����, ������� ���� ������ ��� ��������� �������� ����
	std::set<WordRememberedLong> _wordRememberedLong; // ����� ������� ��� �������� ���� ���������, �� ����������� �����

	AdditionalCheck _additionalCheck;
	MandatoryCheck  _mandatoryCheck;
	LearnNew        _learnNew;
};

