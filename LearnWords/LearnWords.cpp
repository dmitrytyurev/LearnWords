// LearnWords.cpp: определяет точку входа для консольного приложения.
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
// !!! Если время сутки или больше, то вычитается 0.1f 
float addDaysMin[MAX_RIGHT_REPEATS_GLOBAL_N + 1]          = { 0, 0.25f, 1,     1, 1, 3, 6,  8, 15, 25, 90,  90 , 90 , 120 }; // если пользователь привык работать в одно и то же время суток
float addDaysMax[MAX_RIGHT_REPEATS_GLOBAL_N + 1]          = { 0, 0.25f, 1,     1, 1, 4, 7, 12, 20, 30, 100, 100, 100, 140 }; // ему так будет удобнее, иначе каждый день будет сдвиг вперёд
float addDaysMinHardWords[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 1, 1, 1, 1, 4,  6,  8,  15,  25,  40,  80 };  
float addDaysMaxHardWords[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 1, 1, 1, 1, 5,  7,  12, 20,  30,  50,  100 };

const int SECONDS_IN_DAY = 3600 * 24;
const int TIMES_TO_REPEAT_TO_LEARN = 4;  // Сколько раз при изучении показать все слова сразу с переводом, прежде чем начать показывать без перевода
const int TIMES_TO_GUESS_TO_LEARNED = 3;  // Сколько раз нужно правильно назвать значение слова, чтобы оно считалось первично изученным
const int SHOW_WORD_NOT_EARLY_THAN = 10;  // Каким по счёту должно быть это слово при повторном показе (не чаще)
const int PERCENT_FOR_NEAR_WORDS = 30;           // Какой процент слов (от введённого числа) будем повторять из ближайших (смотри описание ADDITIONAL_REPEAT_THRESHOLD_DAYS)
                                                 // Остальной процент слов будем повторять из тех, повтор которых запланирован на более поздний срок
const int ADDITIONAL_REPEAT_THRESHOLD_DAYS = 3;  // Граница в днях между словами, знание которых проверялось недавно и давно
												 // Используется при рандомном выборе слов для проверки, время проверки которых ещё не настало,
												 // но проверка была запрошена пользователем
const int RIGHT_ANSWERS_FALLBACK = 7;            // Номер шага, на который откатывается слово при check_by_time, если помним неуверенно
const int RAND_REPEATS_GLOBAL_N = 8;             // Если rightAnswersNum >= этому значению, то при check_by_time при верном ответе слово помещается в середину очереди рандомной проверки
const int MIN_CLOSE_WORD_LEN = 5;                // Столько первых символов берётся из слов перевода, чтобы искать слова с похожими переводами
const float PERCENT_FORGOT_INSERT_QUEUE = 0.3f;  // Процент от длины очереди неизученных слов куда вставим забытое слово

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
			return (rightAnswersNum == 1) &&   // Это условие не даёт попасть в выборку словам высоких уровней, до повторения которых осталось несколько часов
				((dateOfRepeat >= curTime) && (dateOfRepeat <= curTime + addDaysMax[1] * SECONDS_IN_DAY));  // А это условие не даёт попасть выборку только что изученным словам на следующий день
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
				return false;  // Слово нельзя использовать в случайном повторе потому, что его ещё не учили

			if (cantRandomTestedAfter != 0 && curTime >= cantRandomTestedAfter)  
				return false;  // Слово нельзя использовать в случайном повторе потому, что осталось меньше половины времени до проверки его знания

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
		int randomTestIncID;       // У только что рандомно повторённого слова ставится на 1 больше, чем у всех имеющихся. Если хотим повторить слово раньше (ставим ему такое же значение,
		                           // как у одно из слов примерно в середине списка. Но если позже есть слова, которые тоже вставлялись вне очереди, то от последнего такого слова
		                           // пропускаем одно слово и берём следующее и используем его randomTestIncID (чередуем вставленные слова через 1 со старыми, чтобы они не подвисали)
								   // Если данное слово было вставлено не в конец, то к нему прибавляется RANDOM_REPEAT_FLAG в качестве признака
	};

	struct CompareExcludePair
	{
		std::string word1;
		std::string word2;
	};

	void load_from_file(const char* fullFileName);
	void save_to_file();
	void fill_date_of_repeate_and_save(WordInfo& w, time_t currentTime, bool isHardLearned);

	std::string load_string_from_array(const std::vector<char>& buffer, int* indexToRead);
	int load_int_from_array(const std::vector<char>& buffer, int* indexToRead);
	bool is_digit(char c);

	std::vector<WordInfo> _words;
	std::vector<CompareExcludePair> _compareExcludePairs;
	std::string           _fullFileName;
};

struct WordToLearn
{
	WordToLearn() : _index(0), _counterShown(0), _localRightAnswersNum(0), _wasLastFail(0) {}

	int  _index;                 // Индекс изучаемого слова в WordsOnDisk::_words
	int  _counterShown;          // Запомненное значение счётчика показов слов в момент показа данного слова (используется, чтобы понять было ли показано с тех пор достаточное количество других слов)
	int  _localRightAnswersNum;  // Количество непрерывных правильных ответов
	bool _wasLastFail;           // true, если при последней проверке слова нажали стрелку вниз (забыли слово)
};


WordsOnDisk wordsOnDisk;
time_t curTime = 0;
std::vector<int> forgottenWordsIndices; // Индексы слов, которые были забыты при последней проверке слов

void put_word_to_middle_or_end_of_random_repeat_queue(WordsOnDisk::WordInfo& w, bool isOnlyLoweringID, bool toEndIfSmallQueue);
void calc_words_for_random_repeat(int* totalToRandomRepeat, int* middleQueued);
void log_random_test_wors();
int get_word_to_repeat();
void put_word_to_end_of_random_repeat_queue(WordsOnDisk::WordInfo& w);
void set_word_as_just_hardly_learned(WordsOnDisk::WordInfo& w);

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

char getch_filtered()  // Игнорирует код -32 (встречается, например, у стрелок)
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

	// Читаем пары исключений
	while (true)
	{
		CompareExcludePair cep;

		cep.word1 = load_string_from_array(buffer, &parseIndex);
		if (cep.word1.length() == 0)         // При поиске начала строки был встречен конец файла (например, файл заканчивается пустой строкой)
		{
			printf("Sintax error in word %s", cep.word1.c_str());
			exit(1);
		}

		if (cep.word1 == "Main block")
			break;

		cep.word2 = load_string_from_array(buffer, &parseIndex);

		// Занести CompareExcludePair в вектор
		_compareExcludePairs.push_back(cep);

		while (buffer[parseIndex] != 0 && buffer[parseIndex] != 0xd)
			++parseIndex;

		if (buffer[parseIndex] == 0)
		{
			printf("Sintax error in word");
			exit(1);
		}
	}

	// Читаем основной блок слов
	while (true)
	{
		WordInfo wi;

		wi.word = load_string_from_array(buffer, &parseIndex);
		if (wi.word.length() == 0)         // При поиске начала строки был встречен конец файла (например, файл заканчивается пустой строкой)
			return;
		wi.translation = load_string_from_array(buffer, &parseIndex);

		while (buffer[parseIndex] != 0  &&  buffer[parseIndex] != 0xd && !is_digit(buffer[parseIndex]))
			++parseIndex;

		if (is_digit(buffer[parseIndex]))  // Читаем числовые параметры
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

		// Занести WordInfo в вектор
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
		printf("Can't create file %s\n", _fullFileName.c_str());
		exit(1);
	}

	for (int i = 0; i < _compareExcludePairs.size(); ++i)
	{
		fprintf(f, "\"%s\" \"%s\"\n",
			_compareExcludePairs[i].word1.c_str(),
			_compareExcludePairs[i].word2.c_str());
	}

	fprintf(f, "\"Main block\"\n");

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
	if (isHardLearned)  // Если слово при изучении трудно запоминалось, то используем более плотный график повторений
	{ 
		min = addDaysMinHardWords[w.rightAnswersNum];
		max = addDaysMaxHardWords[w.rightAnswersNum];
	}
	if (min > 0.99f)  min -= 0.1f;  // Если пользователь привык заниматься в одно и то же время каждый день, то нельзя выдавать слова ровно через кратные сутки
	if (max > 0.99f)  max -= 0.1f;  // надо выдавать их чуть раньше.

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


void put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, float percent)
{
	int pos = static_cast<int>(queue.size() * percent);

	if (pos > 0 && rand_int(0, 100) < 50)
		--pos;

	queue.insert(queue.begin() + pos, wordToPut);
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

class CloseTranslationWordsManager  // FIXME!!! избавиться от прямого использования глобального объекта WordsOnDisk. Передавать ссылку на него в методы.
{
public:
	CloseTranslationWordsManager(int wordIndex) : srcWordIndex(wordIndex) {}
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
	void collect_close_words_to(std::vector<CloseWordFound>& closeWordsFound, std::string& rusWord, int srcWordIndex, const std::string& engWord);

private:
	int srcWordIndex;
	std::vector<CloseWordFound> closeWordsFound;
};


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

bool CloseTranslationWordsManager::is_word_in_filter_already(const std::string& word1, const std::string& word2)
{
	for (int i = 0; i < wordsOnDisk._compareExcludePairs.size(); ++i)
	{
		WordsOnDisk::CompareExcludePair& cep = wordsOnDisk._compareExcludePairs[i];
	
		if (cep.word1 == word2  &&  cep.word2 == word1)
			return true;
	}
	return false;
}


bool CloseTranslationWordsManager::is_word_appended_to_translation_already(const std::string& engWord)
{
	WordsOnDisk::WordInfo& wSrc = wordsOnDisk._words[srcWordIndex];

	if (strstr(wSrc.translation.c_str(), engWord.c_str()))
		return true;

	return false;
}

void CloseTranslationWordsManager::collect_close_words_to(std::vector<CloseWordFound>& closeWordsFound, std::string& rusWord, int srcWordIndex, const std::string& engWord)
{
	int length = rusWord.length();
	if (length < MIN_CLOSE_WORD_LEN)
		return;
	int cutLen = std::max(MIN_CLOSE_WORD_LEN, length - 4);

	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
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

		for (int i2 = 0; i2 < words.size(); ++i2)
		{
			if (words[i2].compare(0, cutLen, rusWord, 0, cutLen) == 0)
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

void CloseTranslationWordsManager::print_close_words_by_translation()
{
	puts("\n\n\n\n");

	const WordsOnDisk::WordInfo& w = wordsOnDisk._words[srcWordIndex];

	std::vector<std::string> words;
	get_separate_words_from_translation(w.translation.c_str(), words);

	for (int i=0; i<words.size(); ++i)
		collect_close_words_to(closeWordsFound, words[i], srcWordIndex, w.word);

	for (int i = 0; i<closeWordsFound.size(); ++i)
		printf("%d. %s: %s\n", i+1, closeWordsFound[i].engWord.c_str(), closeWordsFound[i].translation.c_str());
}

void CloseTranslationWordsManager::add_exclusion(int n)
{
	if (n >= closeWordsFound.size())
		return;
	const WordsOnDisk::WordInfo& w = wordsOnDisk._words[srcWordIndex];

	WordsOnDisk::CompareExcludePair cep;
	cep.word1 = w.word;
	cep.word2 = closeWordsFound[n].engWord;
	wordsOnDisk._compareExcludePairs.push_back(cep);
}

void CloseTranslationWordsManager::add_close_eng_word_to_translation(int n)
{
	if (n >= closeWordsFound.size())
		return;
	WordsOnDisk::WordInfo& w = wordsOnDisk._words[srcWordIndex];
	// Найдём место, куда добавить похожее слово. Это место будет сразу после перевода, для которого мы нашли другое англ. слово с близким переводом

	const char* place = strstr(w.translation.c_str(), closeWordsFound[n].rusWordSrc.c_str());
	if (place == nullptr)
	{
		printf("Error 86036939");
		exit(1);
	}

	while (*place != 0 && *place != ',' && *place != '(' && *place != ';')
		++place;

	if (*place == '(')   // Нужно добавить слово в список
	{
		while (*place != 0 && *place != ')')
			++place;
		if (*place == 0)
		{
			printf("Sintax error 5725875");
			exit(1);
		}
		char buf[128] = ", ";
		strcat_s(buf, closeWordsFound[n].engWord.c_str());
		w.translation.insert(place - w.translation.c_str(), buf);
	}
	else                 // Списка ещё нет, создадим его
	{
		char buf[128] = " (";
		strcat_s(buf, closeWordsFound[n].engWord.c_str());
		strcat_s(buf, ")");
		w.translation.insert(place - w.translation.c_str(), buf);
	}
}


void CloseTranslationWordsManager::process_user_input(char c)
{
	std::vector<char> codesToAdd = {49, 50, 51, 52, 53, 54, 55, 56, 57};
	std::vector<char> codesToExclude = {33, 64, 35, 36, 37, 94, 38, 42, 40};

	for (int i=0; i<codesToAdd.size(); ++i)
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


int get_show_word_not_early_than(bool wasLastFail)
{
	if (wasLastFail == false)
		return SHOW_WORD_NOT_EARLY_THAN;

	return static_cast<int>(PERCENT_FORGOT_INSERT_QUEUE * SHOW_WORD_NOT_EARLY_THAN);
}

void learning_words()
{
	forgottenWordsIndices.clear();
	std::vector<int> wordsToLearnIndices;

	// Составим список индексов слов, которые будем учить

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

	// Первичное изучение (показываем неколько раз, постепенно скрывая слово)

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

	// Второй этап - слова показываются без перевода. Если пользователь угадает значение более TIMES_TO_GUESS_TO_LEARNED раз,
	// то слово считается изученным. Цикл изучения заканчивается, когда все слова изучены.

	std::vector<WordToLearn> unlearned;  // Очередь ещё не выученных слов (добавляем в конец, берём из начала)
	std::vector<WordToLearn> learned;    // Очередь выученных слов (добавляем в конец, берём из начала)

	// Занести слова, которые будем изучать в очередь невыученных слов
	for (int i = 0; i < wordsToLearnIndices.size(); ++i)
	{
		const WordsOnDisk::WordInfo& w = wordsOnDisk._words[wordsToLearnIndices[i]];
		WordToLearn word;
		word._index = wordsToLearnIndices[i];
		unlearned.push_back(word);
	}

	// Главный цикл обучения
	int counterWordsShown = SHOW_WORD_NOT_EARLY_THAN;  // Счётчик показанных пользователю слов
	while (true)
	{
		clear_screen();
		print_queue_state(unlearned);
		print_queue_state(learned);

		// Выбрать слово, которое будем показывать
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

		// Показываем слово
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
				case FromWhatSource::FROM_UNLEARNED_QUEUE:
					if ((wordToLearn._wasLastFail == false)  &&  (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED))
					{
						put_to_queue(learned, wordToLearn, 1);
						w.rightAnswersNum = 1;
						wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, false);
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
					put_word_to_end_of_random_repeat_queue(w);
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

	// Выбрать слова для проверки (недавно изученные и забытые)

	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
		if (w.isWordJustLearnedOrForgotten(curTime))
			wordsToRepeat.push_back(i);
	}

	if (wordsToRepeat.size() == 0)
		return;

	// Главный цикл проверки слов

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

		while (true)
		{
			clear_screen();
			printf("\n%s\n", w.word.c_str());
			printf("\n%s\n\n\n  Arrow up   - yes! :)\n  Arrow down - no :(\n", w.translation.c_str());
			CloseTranslationWordsManager ctwm(wordsToRepeat[i]);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх
			{
				w.rightAnswersNum = 1;
				wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, false);
				break;
			}
			else
				if (c == 80) // Стрелка вниз
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

	// Выбрать слова для проверки, для которых подошло время проверки

	for (int i = 0; i < wordsOnDisk._words.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[i];
		if (w.dateOfRepeat != 0 && w.dateOfRepeat < curTime)
		{
			wordsToRepeat.push_back(i);
		}
	}

	// Главный цикл проверки слов

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

		while (true)
		{
			clear_screen();
			printf("%d\n\n%s\n", wordsToRepeat.size() - i, w.word.c_str());
			printf("\n%s\n\n\n  Arrow up   - yes! :)\n  Arrow down - no :(\n  Arrow right - yes, but difficult :(\n", w.translation.c_str());
			CloseTranslationWordsManager ctwm(wordsToRepeat[i]);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх
			{
				if (++w.rightAnswersNum > MAX_RIGHT_REPEATS_GLOBAL_N)
					w.rightAnswersNum = MAX_RIGHT_REPEATS_GLOBAL_N;
				if (w.rightAnswersNum >= RAND_REPEATS_GLOBAL_N)
					put_word_to_middle_or_end_of_random_repeat_queue(w, false, false);
				else
					put_word_to_end_of_random_repeat_queue(w);  // Такие слова и так часто повторяются в check_by_time, поэтому нечего им делать в рандомной проверке. 
				wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, false);
				break;
			}
			else
				if (c == 80) // Стрелка вниз
				{
					forgottenWordsIndices.push_back(wordsToRepeat[i]);
					set_word_as_just_hardly_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, true);
					break;
				}
				else
					if (c == 77) // Стрелка вправо (помним слово не очень уверенно)
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
	std::vector<int> indicesOfWords;   // Индексы подходящих для проверки слов
	fill_indices_of_random_repeat_words(indicesOfWords); // Заполним indicesOfWords
	log("=== Words to random repeat = %d\n", indicesOfWords.size());
	for (int i = 0; i < indicesOfWords.size(); ++i)
	{
		WordsOnDisk::WordInfo& w = wordsOnDisk._words[indicesOfWords[i]];
		log("  %s %d\n", w.word.c_str(), w.randomTestIncID);
	}
}

int get_word_to_repeat()
{
	std::vector<int> indicesOfWords;   // Индексы подходящих для проверки слов
									   
	fill_indices_of_random_repeat_words(indicesOfWords); // Заполним indicesOfWords

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
	std::vector<int> indicesOfWords;   // Индексы подходящих для проверки слов

	fill_indices_of_random_repeat_words(indicesOfWords); // Заполним indicesOfWords индексами слов, подходящих для рандомной проверке в порядке их очереди

	if (indicesOfWords.size() == 0)
		return;

	if (toEndIfSmallQueue  &&  indicesOfWords.size() < 50)   // Если очередь слов на рандомный повтор небольшая, то вставляем в конец, ибо вставка в середину может зациклить повторение одних и тех же слов
	{
		put_word_to_end_of_random_repeat_queue(w);
		return;
	}

	int desiredIndex = static_cast<int>(indicesOfWords.size() * 0.2f);  // У этого слова хотим взять индекс рандомной проверки

	// Но сначала проверим, нет ли слов с бОльшим randomTestIncID, которые тоже вставлялись не в конец, а в середину (если есть, мы должны стать через одно слово после них, не раньше)

	int maxIndex = indicesOfWords.size() - 1;
	while (maxIndex >= 0 && wordsOnDisk._words[indicesOfWords[maxIndex]].hasWordAddedNotAtEnd() == false)
	{
		--maxIndex;
	}

	if (maxIndex != -1 && maxIndex + 1 >= desiredIndex)  // Такое слово нашлось, придётся сдвинуть desiredIndex 
	{
		desiredIndex = std::min(maxIndex + 1, static_cast<int>(indicesOfWords.size()) - 1);  // Пропустим одно слово из тех, что было вставлено не в середину (чтобы такие слова тоже попадали на проверку через одно и не подвисали)
	}

	int desiredIdWithoutFlag = wordsOnDisk._words[indicesOfWords[desiredIndex]].getRandomTestIncIdWithoutFlag();

	if (!onlyLoweringID  ||  desiredIdWithoutFlag < w.getRandomTestIncIdWithoutFlag())  // Обновляем только, если это сдвинет слово в очереди ближе к проверке
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

	// Главный цикл проверки слов

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

		while (true)
		{
			clear_screen();
			printf("%d\n\n%s\n", wordsToRepeatNum - i, w.word.c_str());
			printf("\n%s\n\n\n  Arrow up   - yes! :)\n  Arrow down - no :(\n  Arrow right - yes, but difficult :(\n", w.translation.c_str());
			CloseTranslationWordsManager ctwm(wordToRepeatIndex);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх (помним слово уверенно)
			{
				put_word_to_end_of_random_repeat_queue(w);
				wordsOnDisk.save_to_file();
				break;
			}
			else
				if (c == 80) // Стрелка вниз (забыли слово)
				{
					forgottenWordsIndices.push_back(wordToRepeatIndex);
					set_word_as_just_hardly_learned(w);
					wordsOnDisk.fill_date_of_repeate_and_save(w, curTime, true);
					break;
				}
				else
					if (c == 77) // Стрелка вправо (помним слово не очень уверенно)
					{
						forgottenWordsIndices.push_back(wordToRepeatIndex);
						put_word_to_middle_or_end_of_random_repeat_queue(w, false, true);  // true - чтобы при маленькой очереди слов на рандомный повтор не зацикливались по малому кругу смутные слова
						wordsOnDisk.save_to_file();
						break;
					}
		}
	}
}


int main(int argc, char* argv[])
{
	//char c = getch_filtered();
	//int nRet = GetKeyState(VK_SHIFT);
	//printf("%d %d\n", c, nRet);
	//return 0;

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
		curTime = time(NULL);   // Текущее время обновляется один раз перед показом главного меню, чтобы число слов для повтора в меню 
		                        // и последующем запуске режима повтора (в нём используется запомненный здесь curTime) было одинаковым .
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
  Три очереди:
  - Слова, которые хочется повторять чаще (забытые и полузабытые)





*/