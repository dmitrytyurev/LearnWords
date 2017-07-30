// LearnWords.cpp: определяет точку входа для консольного приложения.
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

const static int MAX_RIGHT_REPEATS_GLOBAL_N = 16;
const static int WORDS_LEARNED_GOOD_THRESHOLD = MAX_RIGHT_REPEATS_GLOBAL_N - 2;
// !!! Если время сутки или больше, то вычитается 0.1f 
float addDaysMin[MAX_RIGHT_REPEATS_GLOBAL_N + 1]          = { 0, 0.25f, 1,     1, 1, 2, 3, 3, 4, 5, 7, 10, 14, 20, 25, 35, 50 }; // если пользователь привык работать в одно и то же время суток
float addDaysMax[MAX_RIGHT_REPEATS_GLOBAL_N + 1]          = { 0, 0.25f, 1,     1, 1, 3, 4, 4, 5, 6, 9, 12, 16, 23, 28, 40, 60 }; // ему так будет удобнее, иначе каждый день будет сдвиг вперёд

const int SECONDS_IN_DAY = 3600 * 24;
const int TIMES_TO_REPEAT_TO_LEARN = 4;  // Сколько раз при изучении показать все слова сразу с переводом, прежде чем начать показывать без перевода
const int TIMES_TO_GUESS_TO_LEARNED = 3;  // Сколько раз нужно правильно назвать значение слова, чтобы оно считалось первично изученным
const int ADDITIONAL_REPEAT_THRESHOLD_DAYS = 3;  // Граница в днях между словами, знание которых проверялось недавно и давно
												 // Используется при рандомном выборе слов для проверки, время проверки которых ещё не настало,
												 // но проверка была запрошена пользователем
const int RIGHT_ANSWERS_FALLBACK = 6;            // Номер шага, на который откатывается слово при check_by_time, если помним неуверенно
const int MIN_CLOSE_WORD_LEN = 5;                // Столько первых символов берётся из слов перевода, чтобы искать слова с похожими переводами
const int REPEAT_AFTER_N_DAYS = 2;    // После попадения в быструю очередь рандомного повтора, слово станет доступным через это число суток
const int QUICK_ANSWER_TIME_MS = 2900;           // Время быстрого ответа в миллисекундах

const char* logFileName = "log.log";

//===============================================================================================
// 
//===============================================================================================

float interp(float x1, float y1, float x2, float y2, float x)
{
	float k = (y2 - y1) / (x2 - x1);
	float b = y1 - x1*k;
	return x*k + b;
}

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
			isInFastRandomQueue = false;
			isNeedSkipOneRandomLoop = false;
		}

		bool isWordJustLearnedOrForgotten(time_t curTime) const
		{
			return (rightAnswersNum == 1) &&   // Это условие не даёт попасть в выборку словам высоких уровней, до повторения которых осталось несколько часов
				((dateOfRepeat >= curTime) && (dateOfRepeat <= curTime + addDaysMax[1] * SECONDS_IN_DAY));  // А это условие не даёт попасть выборку только что изученным словам на следующий день
		}

		bool canRandomTested(time_t curTime) const
		{
			if (rightAnswersNum == 0)
				return false;  // Слово нельзя использовать в случайном повторе потому, что его ещё не учили

			if (cantRandomTestedAfter != 0 && curTime >= cantRandomTestedAfter)  
				return false;  // Слово нельзя использовать в случайном повторе потому, что осталось меньше половины времени до проверки его знания

			if (cantRandomTestedBefore != 0 && curTime <= cantRandomTestedBefore)
				return false;  // Слово нельзя использовать в случайном повторе поскольку мы недавно его повторяли и нужно выждать немного перед ещё одним повтором

			if (isWordJustLearnedOrForgotten(curTime))
				return false; // Слово нельзя использовать в случайном повторе потому, что оно сейчас выводится в списке только что изученных или забытых слов. 

			return true;
		}

		std::string word;
		std::string translation;
		int rightAnswersNum;       // По-дефолту 0 - если слово ещё не учили
								   // Сколько раз слово было успешно проверено. После первичного изучения rightAnswersNum = 1. 
		int dateOfRepeat;          // Дата, когда нужно повторить слово (время в секундах с 1970 года). По-дефолту = 0. Имеет смысл, если rightAnswersNum >= 1
		
		int cantRandomTestedAfter; // Если !=0, то после этого времени нельзя использовать слово в рандомном тесте, ведь скоро уже плановая проверка слова. Иначе будет не чистый тест.
		int randomTestIncID;       // У только что рандомно повторённого слова ставится на 1 больше, чем у всех имеющихся. 
		bool isInFastRandomQueue;  // Находится ли слово в быстрой очереди рандомного повтора
		bool isNeedSkipOneRandomLoop; // Нужно ли пропустить один круг рандомного повтора слова
		int  cantRandomTestedBefore;  // Если !=0, то до этого времени нельзя использовать слово в рандомном тесте, поскольку мы недавно его повторяли и нужно выждать немного перед ещё одним повтором
	};

	struct CompareExcludePair
	{
		std::string word1;
		std::string word2;
	};

	void load_from_file(const char* fullFileName);
	void save_to_file();
	void export_for_google_doc();
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
	WordToLearn() : _index(0), _localRightAnswersNum(0) {}
	WordToLearn(int index) : _index(index), _localRightAnswersNum(0) {}

	int  _index;                 // Индекс изучаемого слова в WordsOnDisk::_words
	int  _localRightAnswersNum;  // Количество непрерывных правильных ответов
};

//===============================================================================================
// 
//===============================================================================================

WordsOnDisk wordsOnDisk;
time_t curTime = 0;
std::vector<int> forgottenWordsIndices; // Индексы слов, которые были забыты при последней проверке слов

//===============================================================================================
// 
//===============================================================================================

void calc_words_for_random_repeat(int* totalToRandomRepeat, int* mainQueueSkipLoopCount, int* middleQueued);
void log_random_test_words();
int get_word_to_repeat();
void put_word_to_end_of_random_repeat_queue_common(WordsOnDisk::WordInfo& w);
void put_word_to_end_of_random_repeat_queue_fast(WordsOnDisk::WordInfo& w, time_t currentTime);
void set_word_as_just_learned(WordsOnDisk::WordInfo& w);
float calc_additional_word_probability(int checkByTimeWordsNumber);

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

char getch_filtered()  // Игнорирует код -32 (встречается, например, у стрелок)
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

void _cdecl exit_msg(char *text, ...)
{
	static char tmpStr[1024];
	va_list args;
	va_start(args, text);
	vsprintf_s(tmpStr, sizeof(tmpStr), text, args);
	va_end(args);

	printf("%s", tmpStr);
	exit(1);
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
		exit_msg("Can't  create file %s\n", logFileName);
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
		exit_msg("Error opening file %s\n", fullFileName);
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer((int)size + 1);
	buffer[(int)size] = 0;
	if (!file.read(buffer.data(), size))
		exit_msg("Error reading file %s\n", fullFileName);

	int parseIndex = 0;

	// Читаем пары исключений
	while (true)
	{
		CompareExcludePair cep;

		cep.word1 = load_string_from_array(buffer, &parseIndex);
		if (cep.word1.length() == 0)         // При поиске начала строки был встречен конец файла (например, файл заканчивается пустой строкой)
			exit_msg("Sintax error in word %s", cep.word1.c_str());

		if (cep.word1 == "Main block")
			break;

		cep.word2 = load_string_from_array(buffer, &parseIndex);

		// Занести CompareExcludePair в вектор
		_compareExcludePairs.push_back(cep);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
			++parseIndex;

		if (buffer[parseIndex] == 0)
			exit_msg("Sintax error in word");
	}

	// Читаем основной блок слов
	while (true)
	{
		WordInfo wi;

		wi.word = load_string_from_array(buffer, &parseIndex);
		if (wi.word.length() == 0)         // При поиске начала строки был встречен конец файла (например, файл заканчивается пустой строкой)
			return;
		wi.translation = load_string_from_array(buffer, &parseIndex);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
			++parseIndex;

		if (is_digit(buffer[parseIndex]))  // Читаем параметры
		{
			wi.rightAnswersNum = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());
			// ---------
			wi.dateOfRepeat = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());
			// ---------
			wi.randomTestIncID = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());

			// ---------
			wi.cantRandomTestedAfter = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());

			// ---------
			wi.isInFastRandomQueue = !!load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());

			// ---------
			wi.isNeedSkipOneRandomLoop = !!load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
				++parseIndex;
			if (buffer[parseIndex] == 0 || buffer[parseIndex] == 0xd)
				exit_msg("Sintax error in word %s", wi.word.c_str());

			// ---------
			wi.cantRandomTestedBefore = load_int_from_array(buffer, &parseIndex);
			while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
				++parseIndex;
		}

		// Занести WordInfo в вектор
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
		exit_msg("Can't create file %s\n", _fullFileName.c_str());

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
			fprintf(f, "\"%s\" \"%s\" %d %d %d %d %d %d %d", 
				e.word.c_str(),
				e.translation.c_str(),
				e.rightAnswersNum,
				e.dateOfRepeat,
				e.randomTestIncID,
				e.cantRandomTestedAfter,
				int(e.isInFastRandomQueue),
				int(e.isNeedSkipOneRandomLoop),
				e.cantRandomTestedBefore);

			fprintf(f, "\n");
		}
	}
	fclose(f);
}

//===============================================================================================
// 
//===============================================================================================

void WordsOnDisk::export_for_google_doc()
{
	const char* fullNameForExpot = "c:\\eng_learn_export.txt";

	FILE* f = NULL;
	fopen_s(&f, fullNameForExpot, "wt");
	if (f == NULL)
		exit_msg("Can't create file %s\n", fullNameForExpot);

	for (const auto& e : _words)
	{
		if (e.rightAnswersNum == 0)
			fprintf(f, "%s#%s\n", e.word.c_str(), e.translation.c_str());
		else
		{
			fprintf(f, "%s#%s#%d#%d#%d#%d#%d#%d#%d",
				e.word.c_str(),
				e.translation.c_str(),
				e.rightAnswersNum,
				e.dateOfRepeat,
				e.randomTestIncID,
				e.cantRandomTestedAfter,
				int(e.isInFastRandomQueue),
				int(e.isNeedSkipOneRandomLoop),
				e.cantRandomTestedBefore);

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
	if (min > 0.99f)  min -= 0.1f;  // Если пользователь привык заниматься в одно и то же время каждый день, то нельзя выдавать слова ровно через кратные сутки
	if (max > 0.99f)  max -= 0.1f;  // надо выдавать их чуть раньше.

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
	printf("\n%s\n\n\n  Стрелка вверх  - помню хорошо!\n  Стрелка вниз   - забыл/перепутал хотя бы одно значение\n", str.c_str());
	if (needRightKeyHint)
		printf("  Стрелка вправо - вспомнил все значения, но с трудом\n");
}

//===============================================================================================
// 
//===============================================================================================

void print_close_words_user_selected(int wordIndex)
{
	//const std::string& closeWord = wordsOnDisk._words[wordIndex].closeWords;   FIXME!!! Сделать чтение отдельных пар перепутанных значений, после этого починить эту функцию

	//for (const auto& curWord : wordsOnDisk._words)
	//	if (curWord.word == closeWord)
	//		printf("%s %s\n", curWord.word.c_str(), curWord.translation.c_str());
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
				exit_msg("Semantic error\n");

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
		if (i >= WORDS_LEARNED_GOOD_THRESHOLD)
			wordsLearnedGood += wordsByLevel[i];
	}
	static int prevWordsLearnedGood;
	int deltaWordsLearnedGood = 0;
	if (prevWordsLearnedGood > 0)
		deltaWordsLearnedGood = wordsLearnedGood - prevWordsLearnedGood;
	prevWordsLearnedGood = wordsLearnedGood;
	printf("Выучено слов: %d, из них хорошо: %d (%d)", wordsLearnedTotal, wordsLearnedGood, deltaWordsLearnedGood);

	int mainQueueLen = 0;
	int mainQueueSkipLoopCount = 0;
	int fastQueueLen = 0;
	calc_words_for_random_repeat(&mainQueueLen, &mainQueueSkipLoopCount, &fastQueueLen);
	
	static int prevSkipLoopCount;
	int deltaSkipLoopCount = 0;
	if (prevSkipLoopCount > 0)
		deltaSkipLoopCount = mainQueueSkipLoopCount - prevSkipLoopCount;
	prevSkipLoopCount = mainQueueSkipLoopCount;
	printf("  Рандомный повтор: Основн=%d (из них skip=%d (%d)), Быстрая=%d ", mainQueueLen, mainQueueSkipLoopCount, deltaSkipLoopCount, fastQueueLen);

	printf("\n");
	printf("\n");
	printf("1. Выучить слова\n");
	printf("2. Повторить только что выученное или забытое [%d]\n", wordsJustLearnedAndForgottenNum);
	printf("3. Дополнительная проверка [N]\n");
	printf("4. Обязательная проверка  [%d+~%d]\n", wordsTimeToRepeatNum, int(wordsTimeToRepeatNum * calc_additional_word_probability(wordsTimeToRepeatNum)));
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

class CloseTranslationWordsManager  // FIXME!!! избавиться от прямого использования глобального объекта WordsOnDisk. Передавать ссылку на него в методы.
{
public:
	explicit CloseTranslationWordsManager(int wordIndex) : srcWordIndex(wordIndex) {}
	void print_close_words_by_translation();
	void process_user_input(char c);

private:
	struct CloseWordFound
	{
		std::string rusWordSrc;  // Для какого русского слова было найдено английское слово с похожим переводом
		std::string engWord;     // Какой английское слово было найдено
		std::string translation; // Полный перевод этого английского слова
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
	// Найдём место, куда добавить похожее слово. Это место будет сразу после перевода, для которого мы нашли другое англ. слово с близким переводом

	size_t pos = wr.find(closeWordsFound[n].rusWordSrc);

	if (pos == std::string::npos)
		exit_msg("Error 86036939");

	while (wr[pos] != 0 && wr[pos] != ',' && wr[pos] != '(' && wr[pos] != ';')
		++pos;

	if (wr[pos] == '(')   // Нужно добавить слово в список
	{
		while (wr[pos] != 0 && wr[pos] != ')')
			++pos;
		if (wr[pos] == 0)
			exit_msg("Sintax error 5725875");
		w.translation.insert(pos, ", " + closeWordsFound[n].engWord);
	}
	else                 // Списка ещё нет, создадим его
		w.translation.insert(pos, " (" + closeWordsFound[n].engWord + ")");
}

//===============================================================================================
// 
//===============================================================================================

void CloseTranslationWordsManager::process_user_input(char c)
{
	char codesToAdd[] = {49, 50, 51, 52, 53, 54, 55, 56, 57};
	char codesToExclude[] = {33, 64, 35, 36, 37, 94, 38, 42, 40};

	const int sizeOfArr = sizeof(codesToAdd) / sizeof(codesToAdd[0]);

	for (int i=0; i<sizeOfArr; ++i)
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
// Вставляем рандомно в последнюю или предпоследнюю позицию 
//===============================================================================================

void put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut)
{
	int pos = (int)queue.size();

	if (pos > 0 && rand_float(0, 1) < 0.2f)
		--pos;

	queue.insert(queue.begin() + pos, wordToPut);
}

//===============================================================================================
// 
//===============================================================================================

bool are_all_words_learned(std::vector<WordToLearn>& queue)
{
	for (const auto& word : queue)
		if (word._localRightAnswersNum < TIMES_TO_GUESS_TO_LEARNED)
			return false;
	return true;
}

//===============================================================================================
// 
//===============================================================================================

void learning_words()
{
	forgottenWordsIndices.clear();
	std::vector<int> wordsToLearnIndices;

	// Составим список индексов слов, которые будем учить

	clear_screen();
	printf("\nСколько слов хотите выучить: ");
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

	// Первичное изучение (показываем неколько раз, постепенно скрывая слово)

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

	// Второй этап - слова показываются без перевода. Если пользователь угадает значение более TIMES_TO_GUESS_TO_LEARNED раз,
	// то слово считается изученным. Цикл изучения заканчивается, когда все слова изучены.

	std::vector<WordToLearn> learnCycleQueue;  // Циклическая очередь слов в процессе изучения (добавляем в конец, берём из начала)

	// Занести слова, которые будем изучать в очередь
	for (const auto& index: wordsToLearnIndices)
	{
		WordToLearn word;
		word._index = index;
		learnCycleQueue.push_back(word);
	}

	// Главный цикл обучения
	while (true)
	{
		clear_screen();

		// Выбрать слово, которое будем показывать
		WordToLearn wordToLearn;
		enum class FromWhatSource
		{
			DEFAULT,
			FROM_QUEUE,
			FROM_RANDOM_REPEAT_LIST,
		} fromWhatSource = FromWhatSource::DEFAULT;

		int wordToRepeatIndex = get_word_to_repeat();

		if (rand_float(0, 1) < 0.6f  ||  wordToRepeatIndex == -1)
		{
			fromWhatSource = FromWhatSource::FROM_QUEUE;
			wordToLearn = learnCycleQueue[0];
			learnCycleQueue.erase(learnCycleQueue.begin());
		}
		else
		{
			fromWhatSource = FromWhatSource::FROM_RANDOM_REPEAT_LIST;
			wordToLearn = WordToLearn(wordToRepeatIndex);
		}

		// Показываем слово
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordToLearn._index];
		printf("\n%s\n", w.word.c_str());
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
		print_buttons_hints(w.translation, false);

		// Обрабатываем ответ - знает ли пользователь слово
		while (true)
		{
			c = getch_filtered();
			if (c == 27)  // ESC
				return;
			if (c == 72)  // Стрелка вверх
			{
				switch (fromWhatSource)
				{
				case FromWhatSource::FROM_QUEUE:
					if (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED)
					{
						set_word_as_just_learned(w);
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
						if (are_all_words_learned(learnCycleQueue))
							return;
					}
					put_to_queue(learnCycleQueue, wordToLearn);
					break;
				case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
					put_word_to_end_of_random_repeat_queue_common(w);
					wordsOnDisk.save_to_file();
					break;
				}
				break;
			}
			else
				if (c == 80) // Стрелка вниз
				{
					switch (fromWhatSource)
					{
					case FromWhatSource::FROM_QUEUE:
						wordToLearn._localRightAnswersNum = 0;
						put_to_queue(learnCycleQueue, wordToLearn);
						w.clear_all();
						wordsOnDisk.save_to_file();
						break;
					case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
						forgottenWordsIndices.push_back(wordToRepeatIndex);
						set_word_as_just_learned(w);
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

void set_word_as_just_learned(WordsOnDisk::WordInfo& w)
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

	// Выбрать слова для проверки (недавно изученные и забытые)

	for (int i = 0; i < (int)wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
		if (w.isWordJustLearnedOrForgotten(curTime))
			wordsToRepeat.push_back(i);
	}

	if (wordsToRepeat.size() == 0)
		return;

	// Главный цикл проверки слов

	std::random_shuffle(wordsToRepeat.begin(), wordsToRepeat.end());
	for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordsToRepeat[i]];

		clear_screen();
		printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
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
			printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
			print_buttons_hints(w.translation, false);
			printf("\n  Осталось: %d\n", (int)wordsToRepeat.size() - i - 1);
			CloseTranslationWordsManager ctwm(wordsToRepeat[i]);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх
			{
				w.rightAnswersNum = 1;
				wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
				break;
			}
			else
				if (c == 80) // Стрелка вниз
				{
					forgottenWordsIndices.push_back(wordsToRepeat[i]);
					set_word_as_just_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
					break;
				}
		}
	}
}

//===============================================================================================
// 
//===============================================================================================

float calc_additional_word_probability(int checkByTimeWordsNumber)
{
	const float MIN_PROB = 0.1f;
	const float MAX_PROB = 0.9f;
	const float MIN_WORDS = 25;
	const float MAX_WORDS = 50;

	float prob = interp(MAX_WORDS, MIN_PROB, MIN_WORDS, MAX_PROB, (float)checkByTimeWordsNumber);   // Рассчитаем вероятность вставки доп. слов в зависимости от числа слов для обязательной проверки по времени
	prob = std::max(MIN_PROB, prob);
	prob = std::min(MAX_PROB, prob);
	return prob;
}

//===============================================================================================
// 
//===============================================================================================

void checking_words_by_time()
{
	enum class FromWhatSource
	{
		DEFAULT,
		CHECK_BY_TIME,
		RANDOM_REPEAT,
	} fromWhatSource = FromWhatSource::DEFAULT;

	struct WordToCheck
	{
		WordToCheck() : _index(0) {}
		WordToCheck(int index, FromWhatSource fromWhatSource): _index(index), _fromWhatSource(fromWhatSource) {}

		int  _index;                     // Индекс повторяемого слова в WordsOnDisk::_words  (если _fromWhatSource == RANDOM_REPEAT, то здесь всегда 0)
		FromWhatSource _fromWhatSource;  // Тип слова
	};

	forgottenWordsIndices.clear();
	clear_screen();

	std::vector<WordToCheck> wordsToRepeat;

	// Выбрать слова для проверки, для которых подошло время проверки

	for (int i = 0; i < (int)wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
		if (w.dateOfRepeat != 0 && w.dateOfRepeat < curTime)
		{
			WordToCheck wordToCheck(i, FromWhatSource::CHECK_BY_TIME);
			wordsToRepeat.push_back(wordToCheck);
		}
	}
	std::random_shuffle(wordsToRepeat.begin(), wordsToRepeat.end());

	// Между слов, которые нужно проверить по времени, повставляем слова для случайной проверки
	float prob = calc_additional_word_probability((int)wordsToRepeat.size());

	for (int i = 0; i < (int)wordsToRepeat.size()-1; ++i)
	{
		if (rand_float(0, 1) < prob)
		{ 
			WordToCheck wordToCheck(0, FromWhatSource::RANDOM_REPEAT);   // Индекс слова не записываем, только его тип! Индекс получим по ходу проверки
			wordsToRepeat.insert(wordsToRepeat.begin() + i + 1, wordToCheck);
			++i;
		}
	}

	// Главный цикл проверки слов

	for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
	{
		if (wordsToRepeat[i]._fromWhatSource == FromWhatSource::RANDOM_REPEAT)
		{
			wordsToRepeat[i]._index = get_word_to_repeat();
			if (wordsToRepeat[i]._index == -1)
				continue;
		}

		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordsToRepeat[i]._index];

		clear_screen();
		printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
		auto t_start = std::chrono::high_resolution_clock::now();

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
			printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
			print_buttons_hints(w.translation, true);
			printf("\n  Осталось: %d, Быстрый ответ = %d\n", (int)wordsToRepeat.size() - i - 1, int(isQuickAnswer));
			CloseTranslationWordsManager ctwm(wordsToRepeat[i]._index);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх
			{
				if (wordsToRepeat[i]._fromWhatSource == FromWhatSource::CHECK_BY_TIME)
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
				}
				else
				{
					put_word_to_end_of_random_repeat_queue_common(w);
					if (isQuickAnswer)
						w.isNeedSkipOneRandomLoop = true;
					wordsOnDisk.save_to_file();
				}
			}
			else
				if (c == 80) // Стрелка вниз
				{
					if (wordsToRepeat[i]._fromWhatSource == FromWhatSource::CHECK_BY_TIME)
					{
						forgottenWordsIndices.push_back(wordsToRepeat[i]._index);
						set_word_as_just_learned(w);
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
					}
					else
					{
						forgottenWordsIndices.push_back(wordsToRepeat[i]._index);
						set_word_as_just_learned(w);
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
					}
				}
				else
					if (c == 77) // Стрелка вправо (помним слово не очень уверенно)
					{
						if (wordsToRepeat[i]._fromWhatSource == FromWhatSource::CHECK_BY_TIME)
						{
							forgottenWordsIndices.push_back(wordsToRepeat[i]._index);
							w.rightAnswersNum = std::min(w.rightAnswersNum, RIGHT_ANSWERS_FALLBACK);
							put_word_to_end_of_random_repeat_queue_fast(w, curTime);
							wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
						}
						else
						{
							forgottenWordsIndices.push_back(wordsToRepeat[i]._index);
							put_word_to_end_of_random_repeat_queue_fast(w, curTime);
							wordsOnDisk.save_to_file();
						}
					}
					else
						continue;
			log("Check by time, word = %s, ===== %s, src=%d, key=%d, time = %s", w.word.c_str(), wordsOnDisk._fullFileName.c_str(), wordsToRepeat[i]._fromWhatSource, c, get_time_in_text(time(NULL)));
			break;
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
	std::vector<int> indicesOfWords;   // Индексы подходящих для проверки слов
	fill_indices_of_random_repeat_words(indicesOfWords, false); // Заполним indicesOfWords
	log("=== Words to random repeat = %d\n", indicesOfWords.size());
	for (const auto& index: indicesOfWords)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[index];
		log("  %s %d skip=%d\n", w.word.c_str(), w.randomTestIncID, w.isNeedSkipOneRandomLoop);
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
	std::vector<int> indicesOfWordsCommon;   // Индексы подходящих для проверки слов из общей очереди
	std::vector<int> indicesOfWordsFast;     // Индексы подходящих для проверки слов из быстрой очереди

	fill_indices_of_random_repeat_words(indicesOfWordsCommon, false);  // Заполним indicesOfWordsCommon
	fill_indices_of_random_repeat_words(indicesOfWordsFast,   true);   // Заполним indicesOfWordsTrue

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
log("put rand common = %d\n", w.randomTestIncID);
	w.isInFastRandomQueue = false;
	w.isNeedSkipOneRandomLoop = false;
	w.cantRandomTestedBefore = 0;
}

//===============================================================================================
// 
//===============================================================================================

void put_word_to_end_of_random_repeat_queue_fast(WordsOnDisk::WordInfo& w, time_t currentTime)
{
	w.randomTestIncID = calc_max_randomTestIncID(true) + 1;
log("put rand fast = %d\n", w.randomTestIncID);
	w.isInFastRandomQueue = true;
	w.isNeedSkipOneRandomLoop = false;
	w.cantRandomTestedBefore = (int)currentTime + int(REPEAT_AFTER_N_DAYS * SECONDS_IN_DAY);;
}

//===============================================================================================
// 
//===============================================================================================

void repeating_random_words()
{
	forgottenWordsIndices.clear();
	clear_screen();
	printf("\nСколько случайных слов хотите повторить: ");
	int wordsToRepeatNum = enter_number_from_console();
	if (wordsToRepeatNum == 0)
		return;

	// Главный цикл проверки слов

	for (int i = 0; i < wordsToRepeatNum; ++i)
	{
		int wordToRepeatIndex = get_word_to_repeat();
		if (wordToRepeatIndex == -1)
			break;
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordToRepeatIndex];

		clear_screen();
		printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
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
			printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
			print_buttons_hints(w.translation, true);
			printf("\n  Осталось: %d, Быстрый ответ = %d\n", wordsToRepeatNum - i - 1, int(isQuickAnswer));
			CloseTranslationWordsManager ctwm(wordToRepeatIndex);
			ctwm.print_close_words_by_translation();
			print_close_words_user_selected(wordToRepeatIndex);
			
			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх (помним слово уверенно)
			{
				put_word_to_end_of_random_repeat_queue_common(w);
				if (isQuickAnswer)
					w.isNeedSkipOneRandomLoop = true;
				wordsOnDisk.save_to_file();
				break;
			}
			else
				if (c == 80) // Стрелка вниз (забыли слово)
				{
					forgottenWordsIndices.push_back(wordToRepeatIndex);
					set_word_as_just_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime);
					break;
				}
				else
					if (c == 77) // Стрелка вправо (помним слово не очень уверенно)
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
//	wordsOnDisk.export_for_google_doc();

//wordsOnDisk.save_to_file();
//return 0;

	while (true)
	{
		clear_screen();
		curTime = time(NULL);   // Текущее время обновляется один раз перед показом главного меню, чтобы число слов для повтора в меню 
		                        // и последующем запуске режима повтора (в нём используется запомненный здесь curTime) было одинаковым .
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

