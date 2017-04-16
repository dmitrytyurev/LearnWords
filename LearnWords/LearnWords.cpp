// LearnWords.cpp: ���������� ����� ����� ��� ����������� ����������.
//

#include "stdafx.h"
#define NOMINMAX
#include "windows.h"
#include "io.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <ctime>
#include <conio.h>
#include <algorithm>
#include <functional>
#include <chrono>

//===============================================================================================
// 
//===============================================================================================

const static int MAX_RIGHT_REPEATS_GLOBAL_N = 13;
// !!! ���� ����� ����� ��� ������, �� ���������� 0.1f 
float addDaysMin[MAX_RIGHT_REPEATS_GLOBAL_N + 1]          = { 0, 0.25f, 1,     1, 1, 3, 6,  8, 15, 25, 90,  90 , 90 , 120 }; // ���� ������������ ������ �������� � ���� � �� �� ����� �����
float addDaysMax[MAX_RIGHT_REPEATS_GLOBAL_N + 1]          = { 0, 0.25f, 1,     1, 1, 4, 7, 12, 20, 30, 100, 100, 100, 140 }; // ��� ��� ����� �������, ����� ������ ���� ����� ����� �����

const int SECONDS_IN_DAY = 3600 * 24;
const int TIMES_TO_REPEAT_TO_LEARN = 4;  // ������� ��� ��� �������� �������� ��� ����� ����� � ���������, ������ ��� ������ ���������� ��� ��������
const int TIMES_TO_GUESS_TO_LEARNED = 3;  // ������� ��� ����� ��������� ������� �������� �����, ����� ��� ��������� �������� ���������
const int SHOW_WORD_NOT_EARLY_THAN = 10;  // ����� �� ����� ������ ���� ��� ����� ��� ��������� ������ (�� ����)
const int PERCENT_FOR_NEAR_WORDS = 30;           // ����� ������� ���� (�� ��������� �����) ����� ��������� �� ��������� (������ �������� ADDITIONAL_REPEAT_THRESHOLD_DAYS)
                                                 // ��������� ������� ���� ����� ��������� �� ���, ������ ������� ������������ �� ����� ������� ����
const int ADDITIONAL_REPEAT_THRESHOLD_DAYS = 3;  // ������� � ���� ����� �������, ������ ������� ����������� ������� � �����
												 // ������������ ��� ��������� ������ ���� ��� ��������, ����� �������� ������� ��� �� �������,
												 // �� �������� ���� ��������� �������������
const int RIGHT_ANSWERS_FALLBACK = 6;            // ����� ����, �� ������� ������������ ����� ��� check_by_time, ���� ������ ����������
const int MIN_CLOSE_WORD_LEN = 5;                // ������� ������ �������� ������ �� ���� ��������, ����� ������ ����� � �������� ����������
const float PERCENT_FORGOT_INSERT_QUEUE = 0.3f;  // ������� �� ����� ������� ����������� ���� ���� ������� ������� �����
const int REPEAT_AFTER_N_DAYS = 2;    // ����� ��������� � ������� ������� ���������� �������, ����� ������ ��������� ����� ��� ����� �����
const int QUICK_ANSWER_TIME_MS = 2500;           // ����� �������� ������ � �������������

const char* logFileName = "log.log";


//===============================================================================================
// 
//===============================================================================================

struct WordsOnDisk
{
	struct WordInfo
	{
		WordInfo() { clear_all(); }
		void clear_all() 
		{	
			rightAnswersNum = 0; 
			dateOfRepeat = 0; 
			cantRandomTestedAfter = 0; 
			randomTestIncID = 0; 
			cantRandomTestedBefore = 0; 
			wordsToTest = 0;
			isInFastRandomQueue = false;
			isNeedSkipOneRandomLoop = false;
		}

		bool isWordJustLearnedOrForgotten(time_t curTime) const
		{
			return (rightAnswersNum == 1) &&   // ��� ������� �� ��� ������� � ������� ������ ������� �������, �� ���������� ������� �������� ��������� �����
				((dateOfRepeat >= curTime) && (dateOfRepeat <= curTime + addDaysMax[1] * SECONDS_IN_DAY));  // � ��� ������� �� ��� ������� ������� ������ ��� ��������� ������ �� ��������� ����
		}

		bool canRandomTested(time_t curTime) const
		{
			if (rightAnswersNum == 0)
				return false;  // ����� ������ ������������ � ��������� ������� ������, ��� ��� ��� �� �����

			if (cantRandomTestedAfter != 0 && curTime >= cantRandomTestedAfter)  
				return false;  // ����� ������ ������������ � ��������� ������� ������, ��� �������� ������ �������� ������� �� �������� ��� ������

			if (cantRandomTestedBefore != 0 && curTime <= cantRandomTestedBefore)
				return false;  // ����� ������ ������������ � ��������� ������� ��������� �� ������� ��� ��������� � ����� ������� ������� ����� ��� ����� ��������

			if (isWordJustLearnedOrForgotten(curTime))
				return false; // ����� ������ ������������ � ��������� ������� ������, ��� ��� ������ ��������� � ������ ������ ��� ��������� ��� ������� ����. 

			return true;
		}

		std::string word;
		std::string translation;
		int rightAnswersNum;       // ��-������� 0 - ���� ����� ��� �� �����
								   // ������� ��� ����� ���� ������� ���������. ����� ���������� �������� rightAnswersNum = 1. 
		int dateOfRepeat;          // ����, ����� ����� ��������� ����� (����� � �������� � 1970 ����). ��-������� = 0. ����� �����, ���� rightAnswersNum >= 1
		
		int cantRandomTestedAfter; // ���� !=0, �� ����� ����� ������� ������ ������������ ����� � ��������� �����, ���� ����� ��� �������� �������� �����. ����� ����� �� ������ ����.
		int randomTestIncID;       // � ������ ��� �������� ����������� ����� �������� �� 1 ������, ��� � ���� ���������. 
		bool isInFastRandomQueue;  // ��������� �� ����� � ������� ������� ���������� �������
		bool isNeedSkipOneRandomLoop; // ����� �� ���������� ���� ���� ���������� ������� �����
		int  cantRandomTestedBefore;  // ���� !=0, �� �� ����� ������� ������ ������������ ����� � ��������� �����, ��������� �� ������� ��� ��������� � ����� ������� ������� ����� ��� ����� ��������
		int  wordsToTest;             // ��� ������ �� ���������� check_word_by_time ������� ������ �������� �������, ����� ���������� ���������� ��� ���������� �������. � ���� ������
		                              // ���������� ������� ���� ���� ����� ������ ������ � ������� ���������� �������. � � ��� ������, ����� ����� ����� ������ �������� ��� ����������
		                              // �������, �� ������� randomTestIncID, ����� ����� ������ ����� ����� ������� �� ���� �������.

		std::string closeWords;    // ���������� ����� ����� ';', ������� ����� �������� ������ � ���� ������ (��������, ������� �� ���������, ������� �� ������� � ������ ������)
	};

	struct CompareExcludePair
	{
		std::string word1;
		std::string word2;
	};

	void load_from_file(const char* fullFileName);
	void load_from_file_legacy(const char* fullFileName);
	void save_to_file();
	void fill_date_of_repeate_and_save(WordInfo& w, time_t currentTime);

	std::string load_string_from_array(const std::vector<char>& buffer, int* indexToRead);
	int load_int_from_array(const std::vector<char>& buffer, int* indexToRead);
	bool is_digit(char c);

	std::vector<WordInfo> _words;
	std::vector<CompareExcludePair> _compareExcludePairs;
	std::string           _fullFileName;
};

//===============================================================================================
// 
//===============================================================================================

struct WordToLearn
{
	WordToLearn() : _index(0), _counterShown(0), _localRightAnswersNum(0), _wasLastFail(0) {}

	int  _index;                 // ������ ���������� ����� � WordsOnDisk::_words
	int  _counterShown;          // ����������� �������� �������� ������� ���� � ������ ������ ������� ����� (������������, ����� ������ ���� �� �������� � ��� ��� ����������� ���������� ������ ����)
	int  _localRightAnswersNum;  // ���������� ����������� ���������� �������
	bool _wasLastFail;           // true, ���� ��� ��������� �������� ����� ������ ������� ���� (������ �����)
};

//===============================================================================================
// 
//===============================================================================================

WordsOnDisk wordsOnDisk;
time_t curTime = 0;
std::vector<int> forgottenWordsIndices; // ������� ����, ������� ���� ������ ��� ��������� �������� ����

//===============================================================================================
// 
//===============================================================================================

void calc_words_for_random_repeat(int* totalToRandomRepeat, int* mainQueueSkipLoopCount, int* middleQueued);
void log_random_test_words();
int get_word_to_repeat();
void put_word_to_end_of_random_repeat_queue_common(WordsOnDisk::WordInfo& w);
void put_word_to_end_of_random_repeat_queue_fast(WordsOnDisk::WordInfo& w, time_t currentTime);
void put_word_to_end_of_random_repeat_queue_common_same_position(WordsOnDisk::WordInfo& w, time_t currentTime);
void set_word_as_just_hardly_learned(WordsOnDisk::WordInfo& w);

//===============================================================================================
// 
//===============================================================================================

const char* get_time_in_text(time_t curTime)
{
	static char buf[128];
	struct tm timeinfo;
	localtime_s(&timeinfo, &curTime);
	asctime_s(buf, sizeof(buf), &timeinfo);
	return buf;
}

//===============================================================================================
// 
//===============================================================================================

inline int rand_int(int min, int max)
{
	return   rand() * (max - min + 1) / (RAND_MAX + 1) + min;
}

//===============================================================================================
// 
//===============================================================================================

inline float rand_float(float min, float max)
{
	return   rand() * (max - min) / RAND_MAX + min;
}

//===============================================================================================
// 
//===============================================================================================

char getch_filtered()  // ���������� ��� -32 (�����������, ��������, � �������)
{
	char c = 0;
	do
		c = _getch();
	while (c == -32);
	return c;
}

//===============================================================================================
// 
//===============================================================================================

void _cdecl log(char *text, ...)
{
	static char tmpStr[1024];
	va_list args;
	va_start(args, text);
	vsprintf_s(tmpStr, sizeof(tmpStr), text, args);
	va_end(args);

	FILE* f = NULL;
	fopen_s(&f, logFileName, "at");
	if (f == NULL)
	{
		printf("Can't  create file %s\n", logFileName);
		exit(1);
	}
	fprintf(f, "%s", tmpStr);
	fclose(f);
}

//===============================================================================================
// 
//===============================================================================================

bool WordsOnDisk::is_digit(char c)
{
	return  c >= '0'  &&  c <= '9';
}

//===============================================================================================
// 
//===============================================================================================

std::string WordsOnDisk::load_string_from_array(const std::vector<char>& buffer, int* indexToRead)
{
	while (buffer[*indexToRead] != '"'  &&  buffer[*indexToRead] != 0)
		++(*indexToRead);
	if (buffer[*indexToRead] == 0)
		return std::string("");
	++(*indexToRead);

	std::string str;
	str.reserve(100);

	while (buffer[*indexToRead] != '"')
	{
		str += buffer[*indexToRead];
		++(*indexToRead);
	}
	++(*indexToRead);

	return str;
}

//===============================================================================================
// 
//===============================================================================================

int WordsOnDisk::load_int_from_array(const std::vector<char>& buffer, int* indexToRead)
{
	int number = 0;

	while (true)
	{
		number += buffer[*indexToRead] - '0';
		++(*indexToRead);
		if (!is_digit(buffer[*indexToRead]))
			return number;
		number *= 10;
	}
	return 0;
}

//===============================================================================================
// 
//===============================================================================================

void WordsOnDisk::load_from_file(const char* fullFileName)
{
	_fullFileName = fullFileName;
	std::ifstream file(fullFileName, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	if (size == -1)
	{
		printf("Error opening file %s\n", fullFileName);
		exit(1);
	}
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer((int)size + 1);
	buffer[(int)size] = 0;
	if (!file.read(buffer.data(), size))
	{
		printf("Error reading file %s\n", fullFileName);
		exit(1);
	}

	int parseIndex = 0;

	// ������ ���� ����������
	while (true)
	{
		CompareExcludePair cep;

		cep.word1 = load_string_from_array(buffer, &parseIndex);
		if (cep.word1.length() == 0)         // ��� ������ ������ ������ ��� �������� ����� ����� (��������, ���� ������������� ������ �������)
		{
			printf("Sintax error in word %s", cep.word1.c_str());
			exit(1);
		}

		if (cep.word1 == "Main block")
			break;

		cep.word2 = load_string_from_array(buffer, &parseIndex);

		// ������� CompareExcludePair � ������
		_compareExcludePairs.push_back(cep);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
			++parseIndex;

		if (buffer[parseIndex] == 0)
		{
			printf("Sintax error in word");
			exit(1);
		}
	}

	// ������ �������� ���� ����
	while (true)
	{
		WordInfo wi;

		wi.word = load_string_from_array(buffer, &parseIndex);
		if (wi.word.length() == 0)         // ��� ������ ������ ������ ��� �������� ����� ����� (��������, ���� ������������� ������ �������)
			return;
		wi.translation = load_string_from_array(buffer, &parseIndex);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
			++parseIndex;

		if (is_digit(buffer[parseIndex]))  // ������ ���������
		{
			wi.rightAnswersNum = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
			{
				printf("Sintax error in word %s", wi.word.c_str());
				exit(1);
			}
			// ---------
			wi.dateOfRepeat = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
			{
				printf("Sintax error in word %s", wi.word.c_str());
				exit(1);
			}
			// ---------
			wi.randomTestIncID = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
			{
				printf("Sintax error in word %s", wi.word.c_str());
				exit(1);
			}

			// ---------
			wi.cantRandomTestedAfter = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
			{
				printf("Sintax error in word %s", wi.word.c_str());
				exit(1);
			}

			// ---------
			wi.isInFastRandomQueue = !!load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
			{
				printf("Sintax error in word %s", wi.word.c_str());
				exit(1);
			}

			// ---------
			wi.isNeedSkipOneRandomLoop = !!load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
			{
				printf("Sintax error in word %s", wi.word.c_str());
				exit(1);
			}

			// ---------
			wi.cantRandomTestedBefore = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
			{
				printf("Sintax error in word %s", wi.word.c_str());
				exit(1);
			}

			// ---------
			wi.wordsToTest = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && buffer[parseIndex] != '"')
				++parseIndex;
			if (buffer[parseIndex] == '"')
			{
				wi.closeWords = load_string_from_array(buffer, &parseIndex);
				while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
					++parseIndex;
			}
		}

		// ������� WordInfo � ������
		_words.push_back(wi);

		if (buffer[parseIndex] == 0)
			return;
	}
}

//===============================================================================================
// 
//===============================================================================================

void WordsOnDisk::save_to_file()
{
	FILE* f = NULL;
	fopen_s(&f, _fullFileName.c_str(), "wt");
	if (f == NULL)
	{
		printf("Can't create file %s\n", _fullFileName.c_str());
		exit(1);
	}

	for (const auto& e:_compareExcludePairs)
	{
		fprintf(f, "\"%s\" \"%s\"\n",
			e.word1.c_str(),
			e.word2.c_str());
	}

	fprintf(f, "\"Main block\"\n");

	for (const auto& e:_words)
	{
		if (e.rightAnswersNum == 0)
			fprintf(f, "\"%s\" \"%s\"\n", e.word.c_str(), e.translation.c_str());
		else
		{
			fprintf(f, "\"%s\" \"%s\" %d %d %d %d %d %d %d %d", 
				e.word.c_str(),
				e.translation.c_str(),
				e.rightAnswersNum,
				e.dateOfRepeat,
				e.randomTestIncID,
				e.cantRandomTestedAfter,
				int(e.isInFastRandomQueue),
				int(e.isNeedSkipOneRandomLoop),
				e.cantRandomTestedBefore,
				e.wordsToTest);

			if (e.closeWords.length())
				fprintf(f, " \"%s\"", e.closeWords.c_str());

			fprintf(f, "\n");
		}
	}
	fclose(f);
}

//===============================================================================================
// 
//===============================================================================================

void WordsOnDisk::fill_date_of_repeate_and_save(WordsOnDisk::WordInfo& w, time_t currentTime)
{
	float min = addDaysMin[w.rightAnswersNum];
	float max = addDaysMax[w.rightAnswersNum];
	if (min > 0.99f)  min -= 0.1f;  // ���� ������������ ������ ���������� � ���� � �� �� ����� ������ ����, �� ������ �������� ����� ����� ����� ������� �����
	if (max > 0.99f)  max -= 0.1f;  // ���� �������� �� ���� ������.

	float randDays = rand_float(min, max);
//printf("%d %f %f %f %d\n", w.rightAnswersNum, min, max, randDays, int(randDays * SECONDS_IN_DAY));
//getch_filtered();
	int secondsPlusCurTime  = int(randDays * SECONDS_IN_DAY);
	w.dateOfRepeat          = (int)currentTime + secondsPlusCurTime;
	w.cantRandomTestedAfter = (int)currentTime + secondsPlusCurTime / 2;
//log("cantRandomTestedAfter = %d, curTime = %ll, secondsPlusCurTime = %d\n", w.cantRandomTestedAfter, currentTime, secondsPlusCurTime);
	save_to_file();
}

//===============================================================================================
// 
//===============================================================================================

void print_buttons_hints(const std::string& str, bool needRightKeyHint)
{
	printf("\n%s\n\n\n  ������� �����  - ����� ������!\n  ������� ����   - �����/��������� ���� �� ���� ��������\n", str.c_str());
	if (needRightKeyHint)
		printf("  ������� ������ - �������� ��� ��������, �� � ������\n");
}

//===============================================================================================
// 
//===============================================================================================

void print_close_words_user_selected(int wordIndex)
{
	const std::string& closeWord = wordsOnDisk._words[wordIndex].closeWords;

	for (const auto& curWord : wordsOnDisk._words)
		if (curWord.word == closeWord)
			printf("%s %s\n", curWord.word.c_str(), curWord.translation.c_str());
}

//===============================================================================================
// 
//===============================================================================================

void clear_screen(char fill = ' ')
{
	COORD tl = { 0,0 };
	CONSOLE_SCREEN_BUFFER_INFO s;
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(console, &s);
	DWORD written, cells = s.dwSize.X * s.dwSize.Y;
	FillConsoleOutputCharacter(console, fill, cells, tl, &written);
	FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
	SetConsoleCursorPosition(console, tl);
}

//===============================================================================================
// 
//===============================================================================================

void recalc_stats(time_t curTime, int* wordsTimeToRepeatNum, int* wordsJustLearnedAndForgottenNum, int wordsByLevel[MAX_RIGHT_REPEATS_GLOBAL_N + 1])
{
	*wordsTimeToRepeatNum = 0;
	*wordsJustLearnedAndForgottenNum = 0;

	for (int i = 0; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
		wordsByLevel[i] = 0;

	for (const auto& w:wordsOnDisk._words)
	{
		++(wordsByLevel[w.rightAnswersNum]);

		if (w.dateOfRepeat != 0)
		{
			if (w.rightAnswersNum == 0)
			{
				printf("Semantic error\n");
				exit(1);
			}

			if (w.dateOfRepeat < curTime)
				++(*wordsTimeToRepeatNum);

			if (w.isWordJustLearnedOrForgotten(curTime))
				++(*wordsJustLearnedAndForgottenNum);
		}
	}

	//	printf("\n%d  - Words that is time to repeat\n\n", wordsTimeToRepeatNum);
}

//===============================================================================================
// 
//===============================================================================================

int main_menu_choose_mode()
{
log_random_test_words();

	int wordsTimeToRepeatNum = 0;
	int wordsJustLearnedAndForgottenNum = 0;
	int wordsByLevel[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
	recalc_stats(curTime, &wordsTimeToRepeatNum, &wordsJustLearnedAndForgottenNum, wordsByLevel);

	int wordsLearnedTotal = 0;
	int wordsLearnedGood  = 0;
	for (int i = 1; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
	{
		wordsLearnedTotal += wordsByLevel[i];
		if (i >= 8)
			wordsLearnedGood += wordsByLevel[i];
	}
	static int prevWordsLearnedGood;
	int deltaWordsLearnedGood = 0;
	if (prevWordsLearnedGood > 0)
		deltaWordsLearnedGood = wordsLearnedGood - prevWordsLearnedGood;
	prevWordsLearnedGood = wordsLearnedGood;
	printf("������� ����: %d, �� ��� ������: %d (delta=%d)", wordsLearnedTotal, wordsLearnedGood, deltaWordsLearnedGood);

	int mainQueueLen = 0;
	int mainQueueSkipLoopCount = 0;
	int fastQueueLen = 0;
	calc_words_for_random_repeat(&mainQueueLen, &mainQueueSkipLoopCount, &fastQueueLen);
	printf("  ��������� ������: ������=%d (�� ��� skip=%d), �������=%d ", mainQueueLen, mainQueueSkipLoopCount, fastQueueLen);

	printf("\n");
	printf("\n");
	printf("1. ������� �����\n");
	printf("2. ��������� ������ ��� ��������� ��� ������� [%d]\n", wordsJustLearnedAndForgottenNum);
	printf("3. ��������� ��������� ����� [N]\n");
	printf("4. ��������� �����, ��� ������� ��������� �����  [%d]\n", wordsTimeToRepeatNum);
	printf("\n\n");

	for (const auto& index:forgottenWordsIndices)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[index];
		printf("==============================================\n%s\n   %s\n", w.word.c_str(), w.translation.c_str());
	}

	while (true)
	{
		char c = getch_filtered();
		if (c == 27 || c == '1' || c == '2' || c == '3' || c == '4')  // 27 - Esc
			return c;
//		printf("%d\n", c);
	}

	return 0;
}

//===============================================================================================
// 
//===============================================================================================

int enter_number_from_console()
{
	char buffer[256] = {0};
	int index = 0;
	int printedSymbolsLastTime = 0;
		
	while (true)
	{
		char c = getch_filtered();
		if (c == 13) // Enter
			break;
		if (c >= '0' && c <= '9')
		{
			buffer[index++] = c;
			buffer[index]   = 0;
		}

		if (c == 8)  // Backspace
		{
			if (index>0)
			  buffer[--index] = 0;
		}

		for (int i=0; i<printedSymbolsLastTime; ++i)
			putchar(8);
		for (int i = 0; i<printedSymbolsLastTime; ++i)
			putchar(' ');
		for (int i = 0; i<printedSymbolsLastTime; ++i)
			putchar(8);
		printf("%s", buffer);
		printedSymbolsLastTime = strlen(buffer);
	}

	int number = 0;
	sscanf_s(buffer, "%d", &number);
	return number;
}

//===============================================================================================
// 
//===============================================================================================

void put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, float percent)
{
	int pos = static_cast<int>(queue.size() * percent);

	if (pos > 0 && rand_int(0, 100) < 50)
		--pos;

	queue.insert(queue.begin() + pos, wordToPut);
}

//===============================================================================================
// 
//===============================================================================================

void wait_time(int waitTimeSec)
{
	time_t t = time(NULL);
	while (time(NULL) - t < waitTimeSec)
	{
		Sleep(100);
	}
}

//===============================================================================================
// 
//===============================================================================================

bool is_russian_symbol(unsigned int c)
{
	if ((c >= 224 && c <= 255) ||
		(c >= 192 && c <= 223) ||
		c == 184 ||
		c == 168)
		return true;
	return false;
}

//===============================================================================================
// 
//===============================================================================================

bool is_symbol(unsigned int c)
{
	if (is_russian_symbol(c))
		return true;

	return (c >= 'a'  &&  c <= 'z') || (c >= 'A'  &&  c <= 'Z');
}

//===============================================================================================
// 
//===============================================================================================

void print_masked_translation(const char* _str, int symbolsToShowNum)
{
	const unsigned char* str = reinterpret_cast<const unsigned char*>(_str);;
	bool isInTranscription = false;
	int  charCounterInWord = 0;
	while (*str)
	{
		if (*str == '[')
			isInTranscription = true;
		if (*str == ']')
			isInTranscription = false;
		bool isInWord = false;
		if (is_symbol(*str) && isInTranscription == false)
		{
			isInWord = true;
			++charCounterInWord;
		}
		else
			charCounterInWord = 0;

		if (isInWord  &&  (charCounterInWord > symbolsToShowNum))
		  printf("_");
		else
			printf("%c", *str);
		++str;
	}
}

//===============================================================================================
// 
//===============================================================================================

class CloseTranslationWordsManager  // FIXME!!! ���������� �� ������� ������������� ����������� ������� WordsOnDisk. ���������� ������ �� ���� � ������.
{
public:
	explicit CloseTranslationWordsManager(int wordIndex) : srcWordIndex(wordIndex) {}
	void print_close_words_by_translation();
	void process_user_input(char c);

private:
	struct CloseWordFound
	{
		std::string rusWordSrc;  // ��� ������ �������� ����� ���� ������� ���������� ����� � ������� ���������
		std::string engWord;     // ����� ���������� ����� ���� �������
		std::string translation; // ������ ������� ����� ����������� �����
	};

	void add_exclusion(int n);
	void add_close_eng_word_to_translation(int n);
	void get_separate_words_from_translation(const char* __str, std::vector<std::string>& outWords);
	bool is_word_in_filter_already(const std::string& word1, const std::string& word2);
	bool is_word_appended_to_translation_already(const std::string& engWord);
	void collect_close_words_to(std::vector<CloseWordFound>& closeWordsFound, const std::string& rusWord, int srcWordIndex, const std::string& engWord);

private:
	int srcWordIndex;
	std::vector<CloseWordFound> closeWordsFound;
};


//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::get_separate_words_from_translation(const char* __str, std::vector<std::string>& outWords)
{
	outWords.clear();
	const unsigned char* _str = reinterpret_cast<const unsigned char*>(__str);
	const unsigned char* str = _str;
	bool isInTranscription = false;
	bool isInRemark = false;
	bool isInWord = false;
	int  beginIndex = 0;
	int  endIndex = 0;
	while (*str)
	{
		if (*str == '[')
			isInTranscription = true;
		if (*str == ']')
			isInTranscription = false;
		if (*str == '(')
			isInRemark = true;
		if (*str == ')')
			isInRemark = false;
		if (is_russian_symbol(*str) && isInTranscription == false && isInRemark == false)
		{
			if (isInWord == false)
				beginIndex = str - _str;
			isInWord = true;
		}
		else
		{
			if (isInWord == true)
			{
				endIndex = str - _str;
				std::string s(__str + beginIndex, endIndex - beginIndex);
				outWords.push_back(s);
			}
			isInWord = false;
		}
		++str;
	}
	if (isInWord == true)
	{
		endIndex = str - _str;
		std::string s(__str + beginIndex, endIndex - beginIndex);
		outWords.push_back(s);
	}
}

//===============================================================================================
// 
//===============================================================================================

bool CloseTranslationWordsManager::is_word_in_filter_already(const std::string& word1, const std::string& word2)
{
	for (const auto& cep:wordsOnDisk._compareExcludePairs)
	{
		if (cep.word1 == word2  &&  cep.word2 == word1)
			return true;
	}
	return false;
}


//===============================================================================================
// 
//===============================================================================================

bool CloseTranslationWordsManager::is_word_appended_to_translation_already(const std::string& engWord)
{
	WordsOnDisk::WordInfo& wSrc = wordsOnDisk._words[srcWordIndex];

	if (wSrc.translation.find(engWord) != std::string::npos)
		return true;

	return false;
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::collect_close_words_to(std::vector<CloseWordFound>& closeWordsFound, const std::string& rusWord, int srcWordIndex, const std::string& engWord)
{
	int length = rusWord.length();
	if (length < MIN_CLOSE_WORD_LEN)
		return;
	int cutLen = std::max(MIN_CLOSE_WORD_LEN, length - 4);

	for (int i = 0; i < (int)wordsOnDisk._words.size(); ++i)
	{
		if (i == srcWordIndex)
			continue;
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];

		if (is_word_in_filter_already(w.word, engWord))
			continue;

		if (is_word_appended_to_translation_already(w.word))
			continue;

		std::vector<std::string> words;
		get_separate_words_from_translation(w.translation.c_str(), words);

		for (const auto& word:words)
		{
			if (word.compare(0, cutLen, rusWord, 0, cutLen) == 0)
			{
				CloseWordFound cwf;
				cwf.engWord     = w.word;
				cwf.rusWordSrc  = rusWord;
				cwf.translation = w.translation;
				closeWordsFound.push_back(cwf);
				break;
			}
		}
	}
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::print_close_words_by_translation()
{
	puts("\n\n\n\n");

	const WordsOnDisk::WordInfo& w = wordsOnDisk._words[srcWordIndex];

	std::vector<std::string> words;
	get_separate_words_from_translation(w.translation.c_str(), words);

	for (const auto& word: words)
		collect_close_words_to(closeWordsFound, word, srcWordIndex, w.word);

	for (int i = 0; i<(int)closeWordsFound.size(); ++i)
		printf("%d. %s: %s\n", i+1, closeWordsFound[i].engWord.c_str(), closeWordsFound[i].translation.c_str());
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::add_exclusion(int n)
{
	if (n >= (int)closeWordsFound.size())
		return;
	const WordsOnDisk::WordInfo& w = wordsOnDisk._words[srcWordIndex];

	WordsOnDisk::CompareExcludePair cep;
	cep.word1 = w.word;
	cep.word2 = closeWordsFound[n].engWord;
	wordsOnDisk._compareExcludePairs.push_back(cep);
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::add_close_eng_word_to_translation(int n)
{
	if (n >= (int)closeWordsFound.size())
		return;
	WordsOnDisk::WordInfo& w = wordsOnDisk._words[srcWordIndex];
	const std::string& wr = w.translation;
	// ����� �����, ���� �������� ������� �����. ��� ����� ����� ����� ����� ��������, ��� �������� �� ����� ������ ����. ����� � ������� ���������

	size_t pos = wr.find(closeWordsFound[n].rusWordSrc);

	if (pos == std::string::npos)
	{
		printf("Error 86036939");
		exit(1);
	}

	while (wr[pos] != 0 && wr[pos] != ',' && wr[pos] != '(' && wr[pos] != ';')
		++pos;

	if (wr[pos] == '(')   // ����� �������� ����� � ������
	{
		while (wr[pos] != 0 && wr[pos] != ')')
			++pos;
		if (wr[pos] == 0)
		{
			printf("Sintax error 5725875");
			exit(1);
		}
		w.translation.insert(pos, ", " + closeWordsFound[n].engWord);
	}
	else                 // ������ ��� ���, �������� ���
		w.translation.insert(pos, " (" + closeWordsFound[n].engWord + ")");
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::process_user_input(char c)
{
	std::vector<char> codesToAdd = {49, 50, 51, 52, 53, 54, 55, 56, 57};
	std::vector<char> codesToExclude = {33, 64, 35, 36, 37, 94, 38, 42, 40};

	for (int i=0; i<(int)codesToAdd.size(); ++i)
		if (codesToAdd[i] == c)
		{
			add_close_eng_word_to_translation(i);
			wordsOnDisk.save_to_file();
		}
		else
			if (codesToExclude[i] == c)
			{
				add_exclusion(i);
				wordsOnDisk.save_to_file();
			}
}


//===============================================================================================
// 
//===============================================================================================

int get_show_word_not_early_than(bool wasLastFail)
{
	if (wasLastFail == false)
		return SHOW_WORD_NOT_EARLY_THAN;

	return static_cast<int>(PERCENT_FORGOT_INSERT_QUEUE * SHOW_WORD_NOT_EARLY_THAN);
}

//===============================================================================================
// 
//===============================================================================================

void learning_words()
{
	forgottenWordsIndices.clear();
	std::vector<int> wordsToLearnIndices;

	// �������� ������ �������� ����, ������� ����� �����

	clear_screen();
	printf("\n������� ���� ������ �������: ");
	int additionalWordsToLearn = enter_number_from_console();

	if (additionalWordsToLearn > 0)
	{
		for (int i = 0; i < (int)wordsOnDisk._words.size(); ++i)
		{
			const WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
			if (w.rightAnswersNum == 0)
			{
				wordsToLearnIndices.push_back(i);
				if (--additionalWordsToLearn == 0)
					break;
			}
		}
	}

	if (wordsToLearnIndices.size() == 0)
		return;

	// ��������� �������� (���������� �������� ���, ���������� ������� �����)

	for (int i2 = 0; i2 < TIMES_TO_REPEAT_TO_LEARN; ++i2)
	{
		for (const auto& index: wordsToLearnIndices)
		{
			const WordsOnDisk::WordInfo& w = wordsOnDisk._words[index];
			char c = 0;

			if (i2 != 0)
			{
				int symbolsToShowNum = 0;
				switch (i2)
				{
				case 0:
					symbolsToShowNum = 100;
					break;
				case 1:
					symbolsToShowNum = 3;
					break;
				case 2:
					symbolsToShowNum = 2;
					break;
				case 3:
					symbolsToShowNum = 1;
					break;
				}

				clear_screen();
				printf("\n%s\n\n", w.word.c_str());
				print_masked_translation(w.translation.c_str(), symbolsToShowNum);
				printf("\n");

				do
				{
					c = getch_filtered();
					if (c == 27)
						return;
				} while (c != ' ');
			}

			clear_screen();
			printf("\n%s\n\n", w.word.c_str());
			printf("%s", w.translation.c_str());
			printf("\n");

			do
			{
				c = getch_filtered();
				if (c == 27)
					return;
			} while (c != ' ');

		}
	}

	// ������ ���� - ����� ������������ ��� ��������. ���� ������������ ������� �������� ����� TIMES_TO_GUESS_TO_LEARNED ���,
	// �� ����� ��������� ���������. ���� �������� �������������, ����� ��� ����� �������.

	std::vector<WordToLearn> unlearned;  // ������� ��� �� ��������� ���� (��������� � �����, ���� �� ������)
	std::vector<WordToLearn> learned;    // ������� ��������� ���� (��������� � �����, ���� �� ������)

	// ������� �����, ������� ����� ������� � ������� ����������� ����
	for (const auto& index: wordsToLearnIndices)
	{
		const WordsOnDisk::WordInfo& w = wordsOnDisk._words[index];
		WordToLearn word;
		word._index = index;
		unlearned.push_back(word);
	}

	// ������� ���� ��������
	int counterWordsShown = SHOW_WORD_NOT_EARLY_THAN;  // ������� ���������� ������������ ����
	while (true)
	{
		clear_screen();

		// ������� �����, ������� ����� ����������
		WordToLearn wordToLearn;
		enum class FromWhatSource
		{
			DEFAULT,
			FROM_UNLEARNED_QUEUE,
			FROM_LEARNED_QUEUE,
			FROM_RANDOM_REPEAT_LIST,
		} fromWhatSource = FromWhatSource::DEFAULT;

		int wordToRepeatIndex = get_word_to_repeat();
		
		if (counterWordsShown - unlearned[0]._counterShown >= get_show_word_not_early_than(unlearned[0]._wasLastFail) ||  (learned.size() == 0  &&  wordToRepeatIndex == -1))
		{
			fromWhatSource = FromWhatSource::FROM_UNLEARNED_QUEUE;
			wordToLearn = unlearned[0];
			unlearned.erase(unlearned.begin());
		}
		else if (wordToRepeatIndex == -1  ||  (learned.size() > 0  &&  counterWordsShown - learned[0]._counterShown >= get_show_word_not_early_than(learned[0]._wasLastFail)))
		{
			fromWhatSource = FromWhatSource::FROM_LEARNED_QUEUE;
			wordToLearn = learned[0];
			learned.erase(learned.begin());
		}
		else
		{
			fromWhatSource = FromWhatSource::FROM_RANDOM_REPEAT_LIST;
			wordToLearn._index = wordToRepeatIndex;
		}

		// ���������� �����
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordToLearn._index];
		wordToLearn._counterShown = ++counterWordsShown;
		printf("\n%s\n", w.word.c_str());
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
		print_buttons_hints(w.translation, false);

		// ������������ ����� - ����� �� ������������ �����
		while (true)
		{
			c = getch_filtered();
			if (c == 27)  // ESC
				return;
			if (c == 72)  // ������� �����
			{
				switch (fromWhatSource)
				{
				case FromWhatSource::FROM_UNLEARNED_QUEUE:
					if ((wordToLearn._wasLastFail == false)  &&  (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED))
					{
						put_to_queue(learned, wordToLearn, 1);
						w.rightAnswersNum = 1;
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
						if (unlearned.size() == 0)
							return;
					}
					else
					{
						wordToLearn._wasLastFail = false;
						put_to_queue(unlearned, wordToLearn, 1);
					}
					break;
				case FromWhatSource::FROM_LEARNED_QUEUE:
					put_to_queue(learned, wordToLearn, 1);
					break;
				case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
					put_word_to_end_of_random_repeat_queue_common(w);
					wordsOnDisk.save_to_file();
					break;
				}
				break;
			}
			else
				if (c == 80) // ������� ����
				{
					switch (fromWhatSource)
					{
					case FromWhatSource::FROM_UNLEARNED_QUEUE:
						wordToLearn._localRightAnswersNum = 0;
						wordToLearn._wasLastFail = true;
						put_to_queue(unlearned, wordToLearn, PERCENT_FORGOT_INSERT_QUEUE);
						break;
					case FromWhatSource::FROM_LEARNED_QUEUE:
						w.clear_all();
						wordsOnDisk.save_to_file();
						wordToLearn._localRightAnswersNum = 0;
						wordToLearn._wasLastFail = true;
						put_to_queue(unlearned, wordToLearn, PERCENT_FORGOT_INSERT_QUEUE);
						break;
					case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
						forgottenWordsIndices.push_back(wordToRepeatIndex);
						set_word_as_just_hardly_learned(w);
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
						break;
					}
					break;
				}
		}
	}
}

//===============================================================================================
// 
//===============================================================================================

void set_word_as_just_hardly_learned(WordsOnDisk::WordInfo& w)
{
	w.clear_all();
	w.rightAnswersNum = 1;
}

//===============================================================================================
// 
//===============================================================================================

void repeating_words_just_learnded_and_forgotten()
{
	forgottenWordsIndices.clear();
	clear_screen();

	std::vector<int> wordsToRepeat;

	// ������� ����� ��� �������� (������� ��������� � �������)

	for (int i = 0; i < (int)wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
		if (w.isWordJustLearnedOrForgotten(curTime))
			wordsToRepeat.push_back(i);
	}

	if (wordsToRepeat.size() == 0)
		return;

	// ������� ���� �������� ����

	std::random_shuffle(wordsToRepeat.begin(), wordsToRepeat.end());
	for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordsToRepeat[i]];

		clear_screen();
		printf("\n\n%s\n", w.word.c_str());
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');

		while (true)
		{
			clear_screen();
			printf("\n\n%s\n", w.word.c_str());
			print_buttons_hints(w.translation, false);
			printf("\n  ��������: %d\n", (int)wordsToRepeat.size() - i - 1);
			CloseTranslationWordsManager ctwm(wordsToRepeat[i]);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // ������� �����
			{
				w.rightAnswersNum = 1;
				wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
				break;
			}
			else
				if (c == 80) // ������� ����
				{
					forgottenWordsIndices.push_back(wordsToRepeat[i]);
					set_word_as_just_hardly_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
					break;
				}
		}
	}
}

//===============================================================================================
// 
//===============================================================================================

void checking_words_by_time()
{
	forgottenWordsIndices.clear();
	clear_screen();

	std::vector<int> wordsToRepeat;

	// ������� ����� ��� ��������, ��� ������� ������� ����� ��������

	for (int i = 0; i < (int)wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
		if (w.dateOfRepeat != 0 && w.dateOfRepeat < curTime)
		{
			wordsToRepeat.push_back(i);
		}
	}

	// ������� ���� �������� ����

	std::random_shuffle(wordsToRepeat.begin(), wordsToRepeat.end());
	for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordsToRepeat[i]];

		clear_screen();
		printf("\n\n%s\n", w.word.c_str());
		auto t_start = std::chrono::high_resolution_clock::now();
log("Check by time, word = %s, ===== %s, time = %s", w.word.c_str(), wordsOnDisk._fullFileName.c_str(), get_time_in_text(curTime));
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');

		auto t_end = std::chrono::high_resolution_clock::now();
		bool isQuickAnswer = (std::chrono::duration<double, std::milli>(t_end - t_start).count()) < QUICK_ANSWER_TIME_MS;
		
		while (true)
		{
			clear_screen();
			printf("\n\n%s\n", w.word.c_str());
			print_buttons_hints(w.translation, true);
			printf("\n  ��������: %d, ������� ����� = %d\n", (int)wordsToRepeat.size() - i - 1, int(isQuickAnswer));
			CloseTranslationWordsManager ctwm(wordsToRepeat[i]);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // ������� �����
			{
				put_word_to_end_of_random_repeat_queue_common(w);
				if (isQuickAnswer)
				{
					w.isNeedSkipOneRandomLoop = true;
					w.rightAnswersNum += 2;
				}
				else
					w.rightAnswersNum += 1;

				w.rightAnswersNum = std::min(w.rightAnswersNum, MAX_RIGHT_REPEATS_GLOBAL_N);
				wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
				break;
			}
			else
				if (c == 80) // ������� ����
				{
					forgottenWordsIndices.push_back(wordsToRepeat[i]);
					set_word_as_just_hardly_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
					break;
				}
				else
					if (c == 77) // ������� ������ (������ ����� �� ����� ��������)
					{
						forgottenWordsIndices.push_back(wordsToRepeat[i]);
						w.rightAnswersNum = std::min(w.rightAnswersNum, RIGHT_ANSWERS_FALLBACK);
						put_word_to_end_of_random_repeat_queue_fast(w, curTime);
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
						break;
					}
		}
	}
}


//===============================================================================================
// 
//===============================================================================================

int calc_max_randomTestIncID(bool isFromFastQueue)
{
	int max = -1;
	for (int i = 0; i < (int)wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];

		if (w.isInFastRandomQueue == isFromFastQueue)
			max = std::max(max, wordsOnDisk._words[i].randomTestIncID);
	}
	return max;
}


//===============================================================================================
// 
//===============================================================================================

void fill_indices_of_random_repeat_words(std::vector<int> &indicesOfWords, bool isFromFastQueue)
{
	indicesOfWords.clear();
	for (int i = 0; i < (int)wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];

		if (w.canRandomTested(curTime)  &&  w.isInFastRandomQueue == isFromFastQueue)
			indicesOfWords.push_back(i);
	}

	std::sort(indicesOfWords.begin(), indicesOfWords.end(), [](int i, int j) { 
		return wordsOnDisk._words[i].randomTestIncID < wordsOnDisk._words[j].randomTestIncID; });
}

//===============================================================================================
// 
//===============================================================================================

void calc_words_for_random_repeat(int* mainQueueLen, int* mainQueueSkipLoopCount, int* fastQueueLen)
{
	std::vector<int> indicesOfWords;
	fill_indices_of_random_repeat_words(indicesOfWords, false);
	*mainQueueLen = indicesOfWords.size();

	*mainQueueSkipLoopCount = 0;
	for (const auto& index : indicesOfWords)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[index];
		if (w.isNeedSkipOneRandomLoop)
			++(*mainQueueSkipLoopCount);
	}

	fill_indices_of_random_repeat_words(indicesOfWords, true);
	*fastQueueLen = indicesOfWords.size();
}

//===============================================================================================
// 
//===============================================================================================

void log_random_test_words()
{
	std::vector<int> indicesOfWords;   // ������� ���������� ��� �������� ����
	fill_indices_of_random_repeat_words(indicesOfWords, false); // �������� indicesOfWords
	log("=== Words to random repeat = %d\n", indicesOfWords.size());
	for (const auto& index: indicesOfWords)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[index];
		log("  %s %d\n", w.word.c_str(), w.randomTestIncID);
	}

	indicesOfWords.clear();
	fill_indices_of_random_repeat_words(indicesOfWords, true);
	log("=== Fast words to random repeat = %d\n", indicesOfWords.size());
	for (const auto& index : indicesOfWords)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[index];
		log("  %s %d\n", w.word.c_str(), w.randomTestIncID);
	}
}

//===============================================================================================
// 
//===============================================================================================

int get_word_to_repeat_inner()
{
	std::vector<int> indicesOfWordsCommon;   // ������� ���������� ��� �������� ���� �� ����� �������
	std::vector<int> indicesOfWordsFast;     // ������� ���������� ��� �������� ���� �� ������� �������

	fill_indices_of_random_repeat_words(indicesOfWordsCommon, false);  // �������� indicesOfWordsCommon
	fill_indices_of_random_repeat_words(indicesOfWordsFast,   true);   // �������� indicesOfWordsTrue

	if ((rand_float(0, 1) < 0.5f || indicesOfWordsCommon.size() == 0) && indicesOfWordsFast.size() > 0)
	{ 
		return indicesOfWordsFast[0];
	}
	else
	{
		if (indicesOfWordsCommon.size() == 0)
			return -1;
		else
			return indicesOfWordsCommon[rand_int(0, std::min(0u, indicesOfWordsCommon.size() - 1))];  // FIXME!!! 4u
	}
}


//===============================================================================================
// 
//===============================================================================================

int get_word_to_repeat()
{
	while (true)
	{
		int index = get_word_to_repeat_inner();
		if (index == -1)
			return index;

		WordsOnDisk::WordInfo& w = wordsOnDisk._words[index];
		if (w.isNeedSkipOneRandomLoop == false)
			return index;

		put_word_to_end_of_random_repeat_queue_common(w);
	}
}

//===============================================================================================
// 
//===============================================================================================

void put_word_to_end_of_random_repeat_queue_common(WordsOnDisk::WordInfo& w)
{
	w.randomTestIncID = calc_max_randomTestIncID(false) + 1;
	w.isInFastRandomQueue = false;
	w.isNeedSkipOneRandomLoop = false;
	w.cantRandomTestedBefore = 0;
	w.wordsToTest = 0;
}

//===============================================================================================
// 
//===============================================================================================

//void put_word_to_end_of_random_repeat_queue_common_same_position(WordsOnDisk::WordInfo& w, time_t currentTime)
//{
//	int keepWordsToTest = w.wordsToTest;
//	put_word_to_end_of_random_repeat_queue_common(w);
//	w.cantRandomTestedBefore = currentTime + int(REPEAT_AFTER_N_DAYS * SECONDS_IN_DAY);;
//
//	if (keepWordsToTest == 0)
//		return;
//
//	std::vector<int> indicesOfWordsCommon;   // ������� ���������� ��� �������� ���� �� ����� �������
//	fill_indices_of_random_repeat_words(indicesOfWordsCommon, false);  // �������� indicesOfWordsCommon
//	
//	int index = std::min(keepWordsToTest, int(indicesOfWordsCommon.size()-1));
//	
//	WordsOnDisk::WordInfo& w2 = wordsOnDisk._words[indicesOfWordsCommon[index]];
//log("same: index=%d, w2.randomTestIncID=%d\n", index, w2.randomTestIncID);
//	w.randomTestIncID = w2.randomTestIncID;
//}

//===============================================================================================
// 
//===============================================================================================

void put_word_to_end_of_random_repeat_queue_fast(WordsOnDisk::WordInfo& w, time_t currentTime)
{
	w.randomTestIncID = calc_max_randomTestIncID(true) + 1;
	w.isInFastRandomQueue = true;
	w.isNeedSkipOneRandomLoop = false;
	w.cantRandomTestedBefore = (int)currentTime + int(REPEAT_AFTER_N_DAYS * SECONDS_IN_DAY);;
	w.wordsToTest = 0;
}

//===============================================================================================
// 
//===============================================================================================

void repeating_random_words()
{
	forgottenWordsIndices.clear();
	clear_screen();
	printf("\n������� ��������� ���� ������ ���������: ");
	int wordsToRepeatNum = enter_number_from_console();
	if (wordsToRepeatNum == 0)
		return;

	// ������� ���� �������� ����

	for (int i = 0; i < wordsToRepeatNum; ++i)
	{
		int wordToRepeatIndex = get_word_to_repeat();
		if (wordToRepeatIndex == -1)
			break;
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordToRepeatIndex];

		clear_screen();
		printf("\n\n%s\n", w.word.c_str());
		auto t_start = std::chrono::high_resolution_clock::now();
log("Random repeat, word = %s, === %s, time = %s", w.word.c_str(), wordsOnDisk._fullFileName.c_str(), get_time_in_text(curTime));
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');

		auto t_end = std::chrono::high_resolution_clock::now();
		bool isQuickAnswer = (std::chrono::duration<double, std::milli>(t_end - t_start).count()) < QUICK_ANSWER_TIME_MS;

		while (true)
		{
			clear_screen();
			printf("\n\n%s\n", w.word.c_str());
			print_buttons_hints(w.translation, true);
			printf("\n  ��������: %d, ������� ����� = %d\n", wordsToRepeatNum - i - 1, int(isQuickAnswer));
			CloseTranslationWordsManager ctwm(wordToRepeatIndex);
			ctwm.print_close_words_by_translation();
			print_close_words_user_selected(wordToRepeatIndex);
			
			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // ������� ����� (������ ����� ��������)
			{
				put_word_to_end_of_random_repeat_queue_common(w);
				if (isQuickAnswer)
					w.isNeedSkipOneRandomLoop = true;
				wordsOnDisk.save_to_file();
				break;
			}
			else
				if (c == 80) // ������� ���� (������ �����)
				{
					forgottenWordsIndices.push_back(wordToRepeatIndex);
					set_word_as_just_hardly_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
					break;
				}
				else
					if (c == 77) // ������� ������ (������ ����� �� ����� ��������)
					{
						forgottenWordsIndices.push_back(wordToRepeatIndex);
						put_word_to_end_of_random_repeat_queue_fast(w, curTime);
						wordsOnDisk.save_to_file();
						break;
					}
		}
	}
}

//===============================================================================================
// � ����, ������� ����� � ���� ������������� ��� ���������� ������� �� ������� cantRandomTestedAfter
// ��������� ���� wordsToTest � ��������� �� ����
//===============================================================================================

//void process_words_became_unreachable_for_random_repeat(time_t currentTime)
//{
//	std::vector<int> indicesOfWordsCommon;   // ������� ���������� ��� �������� ���� �� ����� �������
//
//	bool needToSave = false;
//	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
//	{
//		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
//
//		if (w.isInFastRandomQueue == false &&
//			w.wordsToTest == 0 &&
//			w.cantRandomTestedAfter != 0 &&
//			currentTime > w.cantRandomTestedAfter)
//		{
//			if (indicesOfWordsCommon.size() == 0)
//				fill_indices_of_random_repeat_words(indicesOfWordsCommon, false);  // �������� indicesOfWordsCommon
//
//			int i2 = 0;                                                            // ��������� �� ����� ������� ���� ����� (������� ���� ����� ��� ��� ������)
//			for (i2 = 0; i2 < indicesOfWordsCommon.size(); ++i2)
//			{
//				WordsOnDisk::WordInfo& w2 = wordsOnDisk._words[indicesOfWordsCommon[i2]];
//				if (w2.randomTestIncID >= w.randomTestIncID)
//					break;
//			}
//
//			w.wordsToTest = i2;
//			needToSave = true;
//		}
//	}
//
//	if (needToSave)
//		wordsOnDisk.save_to_file();
//}

//===============================================================================================
// 
//===============================================================================================

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");
	std::srand(unsigned(std::time(0)));
	srand((int)time(NULL));

	if (argc != 2)
	{
		//puts("Ussage:");
		//puts("LearnWords.exe [path to base file]\n");
		//return 0;
		wordsOnDisk.load_from_file("C:\\Dimka\\LearnWords\\dima_to_learn.txt");
	}
	else
		wordsOnDisk.load_from_file(argv[1]);

//wordsOnDisk.save_to_file();
//return 0;

	while (true)
	{
		clear_screen();
		curTime = time(NULL);   // ������� ����� ����������� ���� ��� ����� ������� �������� ����, ����� ����� ���� ��� ������� � ���� 
		                        // � ����������� ������� ������ ������� (� �� ������������ ����������� ����� curTime) ���� ���������� .
//		process_words_became_unreachable_for_random_repeat(curTime);
		int keyPressed = main_menu_choose_mode();
		switch (keyPressed)
		{
		case 27:  // ESC
			printf("%ld\n", int(curTime));
			return 0;
			break;
		case '1':
			learning_words();
			break;
		case '2':
			repeating_words_just_learnded_and_forgotten();
			break;
		case '3':
			repeating_random_words();
			break;
		case '4':
			checking_words_by_time();
			break;
		default:
			break;
		}
	}

    return 0;
}



//curTime = time(NULL);
//for (int i = 0; i < wordsOnDisk._words.size(); ++i)
//{
//	WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
//	w.isHardFirstLearned = 0;
//	w.rightAnswersNum = rand_int(6, 8);
//	wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
//}
//wordsOnDisk.save_to_file();
//return 0;

