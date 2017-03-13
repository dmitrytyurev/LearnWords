// LearnWords.cpp: ���������� ����� ����� ��� ����������� ����������.
//

#include "stdafx.h"
#define NOMINMAX
#include "windows.h"
#include "io.h"
#include <fstream>
#include <vector>
#include <ctime>
#include <conio.h>
#include <algorithm>
#include <functional>

const static int MAX_RIGHT_REPEATS_GLOBAL_N = 13;
// !!! ���� ����� ����� ��� ������, �� ���������� 0.1f 
float addDaysMin[MAX_RIGHT_REPEATS_GLOBAL_N + 1]          = { 0, 0.25f, 1,     1, 1, 3, 6,  8, 15, 25, 90,  90 , 90 , 120 }; // ���� ������������ ������ �������� � ���� � �� �� ����� �����
float addDaysMax[MAX_RIGHT_REPEATS_GLOBAL_N + 1]          = { 0, 0.25f, 1,     1, 1, 4, 7, 12, 20, 30, 100, 100, 100, 140 }; // ��� ��� ����� �������, ����� ������ ���� ����� ����� �����
float addDaysMinHardWords[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 1, 1, 1, 1, 4,  6,  8,  15,  25,  40,  80 };  
float addDaysMaxHardWords[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 1, 1, 1, 1, 5,  7,  12, 20,  30,  50,  100 };

const int SECONDS_IN_DAY = 3600 * 24;
const int TIMES_TO_REPEAT_TO_LEARN = 4;  // ������� ��� ��� �������� �������� ��� ����� ����� � ���������, ������ ��� ������ ���������� ��� ��������
const int TIMES_TO_GUESS_TO_LEARNED = 3;  // ������� ��� ����� ��������� ������� �������� �����, ����� ��� ��������� �������� ���������
const int SHOW_WORD_NOT_EARLY_THAN = 10;  // ����� �� ����� ������ ���� ��� ����� ��� ��������� ������ (�� ����)
const int PERCENT_FOR_NEAR_WORDS = 30;           // ����� ������� ���� (�� ��������� �����) ����� ��������� �� ��������� (������ �������� ADDITIONAL_REPEAT_THRESHOLD_DAYS)
                                                 // ��������� ������� ���� ����� ��������� �� ���, ������ ������� ������������ �� ����� ������� ����
const int ADDITIONAL_REPEAT_THRESHOLD_DAYS = 3;  // ������� � ���� ����� �������, ������ ������� ����������� ������� � �����
												 // ������������ ��� ��������� ������ ���� ��� ��������, ����� �������� ������� ��� �� �������,
												 // �� �������� ���� ��������� �������������
const int RIGHT_ANSWERS_FALLBACK = 7;            // ����� ����, �� ������� ������������ ����� ��� check_by_time, ���� ������ ����������
const int RAND_REPEATS_GLOBAL_N = 8;             // ���� rightAnswersNum >= ����� ��������, �� ��� check_by_time ��� ������ ������ ����� ���������� � �������� ������� ��������� ��������
const int MIN_CLOSE_WORD_LEN = 5;                // ������� ������ �������� ������ �� ���� ��������, ����� ������ ����� � �������� ����������

const char* logFileName = "log.log";

struct WordsOnDisk
{
	struct WordInfo
	{
		static const int RANDOM_REPEAT_FLAG = 1000000000;

		WordInfo() { clear_all(); }
		void clear_all() 
		{	
			rightAnswersNum = 0; dateOfRepeat = 0; cantRandomTestedAfter = 0; randomTestIncID = 0;
		}

		bool isWordJustLearnedOrForgotten(time_t curTime) const
		{
			return (rightAnswersNum == 1) &&   // ��� ������� �� ��� ������� � ������� ������ ������� �������, �� ���������� ������� �������� ��������� �����
				((dateOfRepeat >= curTime) && (dateOfRepeat <= curTime + addDaysMax[1] * SECONDS_IN_DAY));  // � ��� ������� �� ��� ������� ������� ������ ��� ��������� ������ �� ��������� ����
		}

		bool hasWordAddedNotAtEnd() const
		{
			return randomTestIncID >= RANDOM_REPEAT_FLAG;
		}

		int getRandomTestIncIdWithoutFlag() const
		{
			if (randomTestIncID < RANDOM_REPEAT_FLAG)
				return randomTestIncID;
			else
				return randomTestIncID - RANDOM_REPEAT_FLAG;
		}

		bool canRandomTested(time_t curTime) const
		{
			if (rightAnswersNum == 0)
				return false;  // ����� ������ ������������ � ��������� ������� ������, ��� ��� ��� �� �����

			if (cantRandomTestedAfter != 0 && curTime >= cantRandomTestedAfter)  
				return false;  // ����� ������ ������������ � ��������� ������� ������, ��� �������� ������ �������� ������� �� �������� ��� ������

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
		int randomTestIncID;       // � ������ ��� �������� ����������� ����� �������� �� 1 ������, ��� � ���� ���������. ���� ����� ��������� ����� ������ (������ ��� ����� �� ��������,
		                           // ��� � ���� �� ���� �������� � �������� ������. �� ���� ����� ���� �����, ������� ���� ����������� ��� �������, �� �� ���������� ������ �����
		                           // ���������� ���� ����� � ���� ��������� � ���������� ��� randomTestIncID (�������� ����������� ����� ����� 1 �� �������, ����� ��� �� ���������)
								   // ���� ������ ����� ���� ��������� �� � �����, �� � ���� ������������ RANDOM_REPEAT_FLAG � �������� ��������
	};

	void load_from_file(const char* fullFileName);
	void save_to_file();
	void fill_date_of_repeate_and_save(WordInfo& w, time_t currentTime, bool isHardLearned);

	std::string load_string_from_array(const std::vector<char>& buffer, int* indexToRead);
	int load_int_from_array(const std::vector<char>& buffer, int* indexToRead);
	bool is_digit(char c);

	std::vector<WordInfo> _words;
	std::string           _fullFileName;
};

struct WordToLearn
{
	WordToLearn() : _index(0), _counterShown(0), _localRightAnswersNum(0) {}

	int _index;                 // ������ ���������� ����� � WordsOnDisk::_words
	int _counterShown;          // ����������� �������� �������� ������� ���� � ������ ������ ������� ����� (������������, ����� ������ ���� �� �������� � ��� ��� ����������� ���������� ������ ����)
	int _localRightAnswersNum;  // ���������� ����������� ���������� �������
};


WordsOnDisk wordsOnDisk;
time_t curTime = 0;
std::vector<int> forgottenWordsIndices; // ������� ����, ������� ���� ������ ��� ��������� �������� ����

void put_word_to_middle_or_end_of_random_repeat_queue(WordsOnDisk::WordInfo& w, bool isOnlyLoweringID, bool toEndIfSmallQueue);
void calc_words_for_random_repeat(int* totalToRandomRepeat, int* middleQueued);
void log_random_test_wors();
int get_word_to_repeat();
void put_word_to_end_of_random_repeat_queue(WordsOnDisk::WordInfo& w);
void set_word_as_just_hardly_learned(WordsOnDisk::WordInfo& w);
void print_close_words_by_translation(const char* __str, int srcWordIndex);


const char* get_time_in_text(time_t curTime)
{
	static char buf[128];
	struct tm timeinfo;
	localtime_s(&timeinfo, &curTime);
	asctime_s(buf, sizeof(buf), &timeinfo);
	return buf;
}

inline int rand_int(int min, int max)
{
	return   rand() * (max - min + 1) / (RAND_MAX + 1) + min;
}

inline float rand_float(float min, float max)
{
	return   rand() * (max - min) / RAND_MAX + min;
}

char getch_filtered()  // ���������� ��� -32 (�����������, ��������, � �������)
{
	char c = 0;
	do
		c = _getch();
	while (c == -32);
	return c;
}

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


bool WordsOnDisk::is_digit(char c)
{
	return  c >= '0'  &&  c <= '9';
}

std::string WordsOnDisk::load_string_from_array(const std::vector<char>& buffer, int* indexToRead)
{
	while (buffer[*indexToRead] != '"'  &&  buffer[*indexToRead] != 0)
		++(*indexToRead);
	if (buffer[*indexToRead] == 0)
		return std::string("");
	++(*indexToRead);

	char buf[1024];
	int index = 0;

	while (buffer[*indexToRead] != '"')
	{
		buf[index++] = buffer[*indexToRead];
		++(*indexToRead);
	}
	++(*indexToRead);
	buf[index++] = 0;

	std::string str(buf);
	return str;
}

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

	std::vector<char> buffer(size+1);
	buffer[size] = 0;
	if (!file.read(buffer.data(), size))
	{
		printf("Error reading file %s\n", fullFileName);
		exit(1);
	}

	int parseIndex = 0;
	while (true)
	{
		WordInfo wi;

		wi.word = load_string_from_array(buffer, &parseIndex);
		if (wi.word.length() == 0)         // ��� ������ ������ ������ ��� �������� ����� ����� (��������, ���� ������������� ������ �������)
			return;
		wi.translation = load_string_from_array(buffer, &parseIndex);

		while (buffer[parseIndex] != 0  &&  buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
			++parseIndex;

		if (is_digit(buffer[parseIndex]))  // ������ �������� ���������
		{
			wi.rightAnswersNum = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0  &&  buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
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
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
				++parseIndex;
		}

		// ������� WordInfo � ������
		_words.push_back(wi);
//printf("%s = %d\n", wi.word.c_str(), parseIndex);
//printf("%s = %d\n", wi.translation.c_str(), parseIndex);
//printf("%d = %d\n", wi.rightAnswersNum, parseIndex);
//printf("%d = %d\n", wi.dateOfRepeat, parseIndex);

		if (buffer[parseIndex] == 0)
			return;
	}
}

void WordsOnDisk::save_to_file()
{
	FILE* f = NULL;
	fopen_s(&f, _fullFileName.c_str(), "wt");
	if (f == NULL)
	{
		printf("Can't  create file %s\n", _fullFileName.c_str());
		exit(1);
	}
	for (int i = 0; i < _words.size(); ++i)
	{
		if (_words[i].rightAnswersNum == 0  && 
			_words[i].dateOfRepeat == 0 && 
			_words[i].randomTestIncID == 0 &&
			_words[i].cantRandomTestedAfter == 0)
			fprintf(f, "\"%s\" \"%s\"\n", _words[i].word.c_str(), _words[i].translation.c_str());
		else
			fprintf(f, "\"%s\" \"%s\" %d %d %d %d\n", 
				_words[i].word.c_str(),
				_words[i].translation.c_str(),
				_words[i].rightAnswersNum,
				_words[i].dateOfRepeat,
				_words[i].randomTestIncID,
				_words[i].cantRandomTestedAfter);
	}
	fclose(f);
}

void WordsOnDisk::fill_date_of_repeate_and_save(WordsOnDisk::WordInfo& w, time_t currentTime, bool isHardLearned)
{
	float min = addDaysMin[w.rightAnswersNum];
	float max = addDaysMax[w.rightAnswersNum];
	if (isHardLearned)  // ���� ����� ��� �������� ������ ������������, �� ���������� ����� ������� ������ ����������
	{ 
		min = addDaysMinHardWords[w.rightAnswersNum];
		max = addDaysMaxHardWords[w.rightAnswersNum];
	}
	if (min > 0.99f)  min -= 0.1f;  // ���� ������������ ������ ���������� � ���� � �� �� ����� ������ ����, �� ������ �������� ����� ����� ����� ������� �����
	if (max > 0.99f)  max -= 0.1f;  // ���� �������� �� ���� ������.

	float randDays = rand_float(min, max);
//printf("%d %f %f %f %d\n", w.rightAnswersNum, min, max, randDays, int(randDays * SECONDS_IN_DAY));
//getch_filtered();
	int secondsPlusCurTime  = int(randDays * SECONDS_IN_DAY);
	w.dateOfRepeat          = currentTime + secondsPlusCurTime;
	w.cantRandomTestedAfter = currentTime + secondsPlusCurTime / 2;
//log("cantRandomTestedAfter = %d, curTime = %ll, secondsPlusCurTime = %d\n", w.cantRandomTestedAfter, currentTime, secondsPlusCurTime);
	save_to_file();
}

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

void recalc_stats(time_t curTime, int* wordsTimeToRepeatNum, int* wordsJustLearnedAndForgottenNum, int wordsByLevel[MAX_RIGHT_REPEATS_GLOBAL_N + 1])
{
	*wordsTimeToRepeatNum = 0;
	*wordsJustLearnedAndForgottenNum = 0;

	for (int i = 0; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
		wordsByLevel[i] = 0;

	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
	{
		const WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];

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

int main_menu_choose_mode()
{
log_random_test_wors();

	int wordsTimeToRepeatNum = 0;
	int wordsJustLearnedAndForgottenNum = 0;
	int wordsByLevel[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
	recalc_stats(curTime, &wordsTimeToRepeatNum, &wordsJustLearnedAndForgottenNum, wordsByLevel);

	int wordsLearnedGood = 0;
	for (int i = 1; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
	{
		printf("%d ", wordsByLevel[i]);
		if (i >= 8)
			wordsLearnedGood += wordsByLevel[i];
	}
	printf("  Learned good: %d ", wordsLearnedGood);

	int totalToRandomRepeat = 0;
	int middleQueued = 0;
	calc_words_for_random_repeat(&totalToRandomRepeat, &middleQueued);
	printf("  rToRR: %d, aNAE: %d ", totalToRandomRepeat, middleQueued);

	printf("\n");
	printf("\n");
	printf("1. Learn words\n");
	printf("2. Repeat words just learned or forgotten [%d]\n", wordsJustLearnedAndForgottenNum);
	printf("3. Repeat random words [select]\n");
	printf("4. Check words by time [%d]\n", wordsTimeToRepeatNum);
	printf("\n\n");

	for (int i = 0; i < forgottenWordsIndices.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[forgottenWordsIndices[i]];
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


void put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut)
{
	// ��������� �� � �������������, ���� � ��������� ������� ��������
	if (queue.size() > 0 && rand_int(0, 100) < 50)
		queue.insert(queue.begin() + (queue.size() - 1), wordToPut);
	else
		queue.push_back(wordToPut);
}

void print_queue_state(const std::vector<WordToLearn>& queue)
{
return;
	printf("=== Queue ===\n");
	log("=== Queue ===\n");
	for (int i = queue.size()-1; i >= 0; --i)
	{
		printf("%s rightAnswersNum=%d counter=%d\n", wordsOnDisk._words[queue[i]._index].word.c_str(), queue[i]._localRightAnswersNum, queue[i]._counterShown);
		log("%s rightAnswersNum=%d counter=%d\n", wordsOnDisk._words[queue[i]._index].word.c_str(), queue[i]._localRightAnswersNum, queue[i]._counterShown);
	}
	printf("\n");
	log("\n");
}


void wait_time(int waitTimeSec)
{
	time_t t = time(NULL);
	while (time(NULL) - t < waitTimeSec)
	{
		Sleep(100);
	}
}

bool is_russian_symbol(unsigned int c)
{
	if ((c >= 224 && c <= 255) ||
		(c >= 192 && c <= 223) ||
		c == 184 ||
		c == 168)
		return true;
	return false;
}

bool is_symbol(unsigned int c)
{
	if (is_russian_symbol(c))
		return true;

	return (c >= 'a'  &&  c <= 'z') || (c >= 'A'  &&  c <= 'Z');
}



void print_masked_translation(const char* _str, int symbolsToShowNum)
{
	const unsigned char* str = reinterpret_cast<const unsigned char*>(_str);;
	bool isInTranscription = false;
	bool isInRemark        = false;
	int  charCounterInWord = 0;
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

void print_close_words_to(const char* word, int length, int srcWordIndex)
{
	if (length < MIN_CLOSE_WORD_LEN)
		return;
	char wordPartToFind[128];

	strncpy_s(wordPartToFind, word, length);
	wordPartToFind[length] = 0;
	wordPartToFind[MIN_CLOSE_WORD_LEN] = 0;

	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
	{
		if (i == srcWordIndex)
			continue;
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];

		if (strstr(w.translation.c_str(), wordPartToFind))
			printf("%s %s\n", w.word.c_str(), w.translation.c_str());
	}
}

void print_close_words_by_translation(const char* __str, int srcWordIndex)
{
	puts("\n\n\n\n");
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
				print_close_words_to(__str + beginIndex, endIndex - beginIndex, srcWordIndex);
			}
			isInWord = false;
		}
		++str;
	}
	if (isInWord == true)
	{
		endIndex = str - _str;
		print_close_words_to(__str + beginIndex, endIndex - beginIndex, srcWordIndex);
	}
}

void learning_words()
{
	forgottenWordsIndices.clear();
	std::vector<int> wordsToLearnIndices;

	// �������� ������ �������� ����, ������� ����� �����

	clear_screen();
	printf("\nPlease, enter the number of words to learn: ");
	int additionalWordsToLearn = enter_number_from_console();

	if (additionalWordsToLearn > 0)
	{
		for (int i = 0; i < wordsOnDisk._words.size(); ++i)
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
		for (int i = 0; i < wordsToLearnIndices.size(); ++i)
		{
			const WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordsToLearnIndices[i]];
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
	for (int i = 0; i < wordsToLearnIndices.size(); ++i)
	{
		const WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordsToLearnIndices[i]];
		WordToLearn word;
		word._index = wordsToLearnIndices[i];
		unlearned.push_back(word);
	}

	// ������� ���� ��������
	int counterWordsShown = SHOW_WORD_NOT_EARLY_THAN;  // ������� ���������� ������������ ����
	while (true)
	{
		clear_screen();
		print_queue_state(unlearned);
		print_queue_state(learned);

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

		if (counterWordsShown - unlearned[0]._counterShown >= SHOW_WORD_NOT_EARLY_THAN  ||  (learned.size() == 0  &&  wordToRepeatIndex == -1))
		{
			fromWhatSource = FromWhatSource::FROM_UNLEARNED_QUEUE;
			wordToLearn = unlearned[0];
			unlearned.erase(unlearned.begin());
		}
		else if (wordToRepeatIndex == -1  ||  (learned.size() > 0  &&  counterWordsShown - learned[0]._counterShown >= SHOW_WORD_NOT_EARLY_THAN))
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
		printf("\n%s\n\n\n  Arrow up   - yes! :)\n  Arrow down - no :(\n", w.translation.c_str());

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
					if (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED)
					{
						put_to_queue(learned, wordToLearn);
						w.rightAnswersNum = 1;
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, false);
						if (unlearned.size() == 0)
							return;
					}
					else
						put_to_queue(unlearned, wordToLearn);
					break;
				case FromWhatSource::FROM_LEARNED_QUEUE:
					put_to_queue(learned, wordToLearn);
					break;
				case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
					put_word_to_end_of_random_repeat_queue(w);
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
						put_to_queue(unlearned, wordToLearn);
						break;
					case FromWhatSource::FROM_LEARNED_QUEUE:
						w.clear_all();
						wordsOnDisk.save_to_file();
						wordToLearn._localRightAnswersNum = 0;
						put_to_queue(unlearned, wordToLearn);
						break;
					case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
						forgottenWordsIndices.push_back(wordToRepeatIndex);
						set_word_as_just_hardly_learned(w);
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, true);
						break;
					}
					break;
				}
		}
	}
}

void set_word_as_just_hardly_learned(WordsOnDisk::WordInfo& w)
{
	w.clear_all();
	w.rightAnswersNum = 1;
}

void repeating_words_just_learnded_and_forgotten()
{
	forgottenWordsIndices.clear();
	clear_screen();

	std::vector<int> wordsToRepeat;

	// ������� ����� ��� �������� (������� ��������� � �������)

	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
		if (w.isWordJustLearnedOrForgotten(curTime))
			wordsToRepeat.push_back(i);
	}

	if (wordsToRepeat.size() == 0)
		return;

	// ������� ���� �������� ����

	std::random_shuffle(wordsToRepeat.begin(), wordsToRepeat.end());
	for (int i = 0; i < wordsToRepeat.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordsToRepeat[i]];

		clear_screen();
		printf("\n%s\n", w.word.c_str());
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
		printf("\n%s\n\n\n  Arrow up   - yes! :)\n  Arrow down - no :(\n", w.translation.c_str());
		print_close_words_by_translation(w.translation.c_str(), wordsToRepeat[i]);

		while (true)
		{
			c = getch_filtered();
			if (c == 27)
				return;
			if (c == 72)  // ������� �����
			{
				w.rightAnswersNum = 1;
				wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, false);
				break;
			}
			else
				if (c == 80) // ������� ����
				{
					forgottenWordsIndices.push_back(wordsToRepeat[i]);
					set_word_as_just_hardly_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, true);
					break;
				}
		}
	}
}

void checking_words_by_time()
{
	forgottenWordsIndices.clear();
	clear_screen();

	std::vector<int> wordsToRepeat;

	// ������� ����� ��� ��������, ��� ������� ������� ����� ��������

	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
		if (w.dateOfRepeat != 0 && w.dateOfRepeat < curTime)
		{
			wordsToRepeat.push_back(i);
		}
	}

	// ������� ���� �������� ����

	std::random_shuffle(wordsToRepeat.begin(), wordsToRepeat.end());
	for (int i = 0; i < wordsToRepeat.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordsToRepeat[i]];

		clear_screen();
		printf("%d\n\n%s\n", wordsToRepeat.size()-i, w.word.c_str());
log("Check by time, word = %s, ===== %s, time = %s", w.word.c_str(), wordsOnDisk._fullFileName.c_str(), get_time_in_text(curTime));
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
		printf("\n%s\n\n\n  Arrow up   - yes! :)\n  Arrow down - no :(\n  Arrow right - yes, but difficult :(\n", w.translation.c_str());
		print_close_words_by_translation(w.translation.c_str(), wordsToRepeat[i]);

		while (true)
		{
			c = getch_filtered();
			if (c == 27)
				return;
			if (c == 72)  // ������� �����
			{
				if (++w.rightAnswersNum > MAX_RIGHT_REPEATS_GLOBAL_N)
					w.rightAnswersNum = MAX_RIGHT_REPEATS_GLOBAL_N;
				if (w.rightAnswersNum >= RAND_REPEATS_GLOBAL_N)
					put_word_to_middle_or_end_of_random_repeat_queue(w, false, false);
				else
					put_word_to_end_of_random_repeat_queue(w);  // ����� ����� � ��� ����� ����������� � check_by_time, ������� ������ �� ������ � ��������� ��������. 
				wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, false);
				break;
			}
			else
				if (c == 80) // ������� ����
				{
					forgottenWordsIndices.push_back(wordsToRepeat[i]);
					set_word_as_just_hardly_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, true);
					break;
				}
				else
					if (c == 77) // ������� ������ (������ ����� �� ����� ��������)
					{
						forgottenWordsIndices.push_back(wordsToRepeat[i]);
						w.rightAnswersNum = std::min(w.rightAnswersNum, RIGHT_ANSWERS_FALLBACK);
						put_word_to_middle_or_end_of_random_repeat_queue(w, false, false);
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, true);
						break;
					}
		}
	}
}


int calc_max_randomTestIncID()
{
	int max = wordsOnDisk._words[0].getRandomTestIncIdWithoutFlag();
	for (int i = 1; i < wordsOnDisk._words.size(); ++i)
	{
		max = std::max(max, wordsOnDisk._words[i].getRandomTestIncIdWithoutFlag());
	}
	return max;
}


void calc_words_for_random_repeat(int* totalToRandomRepeat, int* middleQueued)
{
	*totalToRandomRepeat = 0;
	*middleQueued = 0;

	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];

		if (w.canRandomTested(curTime))
		{
			++(*totalToRandomRepeat);

			if (w.hasWordAddedNotAtEnd())
				++(*middleQueued);
		}
	}
}

void fill_indices_of_random_repeat_words(std::vector<int> &indicesOfWords)
{
	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];

		if (w.canRandomTested(curTime))
			indicesOfWords.push_back(i);
	}

	std::sort(indicesOfWords.begin(), indicesOfWords.end(), [](int i, int j) { 
		int ival = wordsOnDisk._words[i].getRandomTestIncIdWithoutFlag();
		int jval = wordsOnDisk._words[j].getRandomTestIncIdWithoutFlag();
		if (ival == jval)
			return wordsOnDisk._words[i].randomTestIncID < wordsOnDisk._words[j].randomTestIncID;
		return  ival < jval; });
}

void log_random_test_wors()
{
	std::vector<int> indicesOfWords;   // ������� ���������� ��� �������� ����
	fill_indices_of_random_repeat_words(indicesOfWords); // �������� indicesOfWords
	log("=== Words to random repeat = %d\n", indicesOfWords.size());
	for (int i = 0; i < indicesOfWords.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[indicesOfWords[i]];
		log("  %s %d\n", w.word.c_str(), w.randomTestIncID);
	}
}

int get_word_to_repeat()
{
	std::vector<int> indicesOfWords;   // ������� ���������� ��� �������� ����
									   
	fill_indices_of_random_repeat_words(indicesOfWords); // �������� indicesOfWords

	if (indicesOfWords.size() == 0)
		return -1;

	return indicesOfWords[rand_int(0, std::min(4u, indicesOfWords.size() - 1))];
}

void put_word_to_end_of_random_repeat_queue(WordsOnDisk::WordInfo& w)
{
	w.randomTestIncID = calc_max_randomTestIncID() + 1;
}

void put_word_to_middle_or_end_of_random_repeat_queue(WordsOnDisk::WordInfo& w, bool onlyLoweringID, bool toEndIfSmallQueue)
{
	std::vector<int> indicesOfWords;   // ������� ���������� ��� �������� ����

	fill_indices_of_random_repeat_words(indicesOfWords); // �������� indicesOfWords ��������� ����, ���������� ��� ��������� �������� � ������� �� �������

	if (indicesOfWords.size() == 0)
		return;

	if (toEndIfSmallQueue  &&  indicesOfWords.size() < 50)   // ���� ������� ���� �� ��������� ������ ���������, �� ��������� � �����, ��� ������� � �������� ����� ��������� ���������� ����� � ��� �� ����
	{
		put_word_to_end_of_random_repeat_queue(w);
		return;
	}

	int desiredIndex = static_cast<int>(indicesOfWords.size() * 0.2f);  // � ����� ����� ����� ����� ������ ��������� ��������

	// �� ������� ��������, ��� �� ���� � ������� randomTestIncID, ������� ���� ����������� �� � �����, � � �������� (���� ����, �� ������ ����� ����� ���� ����� ����� ���, �� ������)

	int maxIndex = indicesOfWords.size() - 1;
	while (maxIndex >= 0 && wordsOnDisk._words[indicesOfWords[maxIndex]].hasWordAddedNotAtEnd() == false)
	{
		--maxIndex;
	}

	if (maxIndex != -1 && maxIndex + 1 >= desiredIndex)  // ����� ����� �������, ������� �������� desiredIndex 
	{
		desiredIndex = std::min(maxIndex + 1, static_cast<int>(indicesOfWords.size()) - 1);  // ��������� ���� ����� �� ���, ��� ���� ��������� �� � �������� (����� ����� ����� ���� �������� �� �������� ����� ���� � �� ���������)
	}

	int desiredIdWithoutFlag = wordsOnDisk._words[indicesOfWords[desiredIndex]].getRandomTestIncIdWithoutFlag();

	if (!onlyLoweringID  ||  desiredIdWithoutFlag < w.getRandomTestIncIdWithoutFlag())  // ��������� ������, ���� ��� ������� ����� � ������� ����� � ��������
		w.randomTestIncID = WordsOnDisk::WordInfo::RANDOM_REPEAT_FLAG + desiredIdWithoutFlag;
}


void repeating_random_words()
{
	forgottenWordsIndices.clear();
	clear_screen();
	printf("\nPlease, enter the number of RANDOM words to repeat: ");
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
		printf("%d\n\n%s\n", wordsToRepeatNum-i, w.word.c_str());
log("Random repeat, word = %s, === %s, time = %s", w.word.c_str(), wordsOnDisk._fullFileName.c_str(), get_time_in_text(curTime));
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
		printf("\n%s\n\n\n  Arrow up   - yes! :)\n  Arrow down - no :(\n  Arrow right - yes, but difficult :(\n", w.translation.c_str());
		print_close_words_by_translation(w.translation.c_str(), wordToRepeatIndex);

		while (true)
		{
			c = getch_filtered();
			if (c == 27)
				return;
			if (c == 72)  // ������� ����� (������ ����� ��������)
			{
				put_word_to_end_of_random_repeat_queue(w);
				wordsOnDisk.save_to_file();
				break;
			}
			else
				if (c == 80) // ������� ���� (������ �����)
				{
					forgottenWordsIndices.push_back(wordToRepeatIndex);
					set_word_as_just_hardly_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, true);
					break;
				}
				else
					if (c == 77) // ������� ������ (������ ����� �� ����� ��������)
					{
						forgottenWordsIndices.push_back(wordToRepeatIndex);
						put_word_to_middle_or_end_of_random_repeat_queue(w, false, true);  // true - ����� ��� ��������� ������� ���� �� ��������� ������ �� ������������� �� ������ ����� ������� �����
						wordsOnDisk.save_to_file();
						break;
					}
		}
	}
}


int main(int argc, char* argv[])
{
	std::srand(unsigned(std::time(0)));
	srand(time(NULL));

	if (argc != 2)
	{
		//puts("Ussage:");
		//puts("LearnWords.exe [path to base file]\n");
		//return 0;
		wordsOnDisk.load_from_file("C:\\Dimka\\LearnWords\\dima_to_learn.txt");
	}
	else
		wordsOnDisk.load_from_file(argv[1]);

	setlocale(LC_ALL, "Russian");
	while (true)
	{
		clear_screen();
		curTime = time(NULL);   // ������� ����� ����������� ���� ��� ����� ������� �������� ����, ����� ����� ���� ��� ������� � ���� 
		                        // � ����������� ������� ������ ������� (� �� ������������ ����������� ����� curTime) ���� ���������� .
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



//std::function<int(int, int)> f1 = [](int left, int right) { return left + right; };
//std::function<int(int, int)> f2 = [](int left, int right) { return left - right; };
//
//void foo(std::function<int(int, int)> inf)
//{
//	printf("%d\n", inf(10, 3));
//}
//foo(f1);
//foo(f2);
//return 0;

//if (++(wordToLearn._localWrongAnswersNum) == WRONG_ANSWERS_FOR_HARD_LEARNING)
//w.isHardFirstLearned = 1;

/*
  ��� �������:
  - �����, ������� ������� ��������� ���� (������� � �����������)





*/