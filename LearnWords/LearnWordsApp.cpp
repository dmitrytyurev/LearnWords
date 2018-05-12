#include "stdafx.h"
#include "CommonUtility.h"
#include "LearnWordsApp.h"
#include "FileOperate.h"
#include "Windows.h"
#undef min

const int QUICK_ANSWER_TIME_MS_FOR_ONE_TRANSLATION   = 2200;   // ¬рем€ быстрого ответа в миллисекундах дл€ слова имеющего одно значение
const int QUICK_ANSWER_TIME_MS_FOR_MORE_TRANSLATIONS = 3200;   // ¬рем€ быстрого ответа в миллисекундах дл€ слова имеющего более одного значени€
const int MAX_RIGHT_REPEATS_GLOBAL_N = 81;
const int WORDS_LEARNED_GOOD_THRESHOLD = 22; // „исло дней в addDaysMin, по которому выбираетс€ индекс, чтобы считать слова хорошо изученными
const int DOWN_ANSWERS_FALLBACK = 20;             // Ќомер шага, на который откатываетс€ слово при check_by_time, если забыли слово
const int RIGHT_ANSWERS_FALLBACK = 29;            // Ќомер шага, на который откатываетс€ слово при check_by_time, если помним неуверенно
const float MIN_DAYS_IF_QUICK_ANSWER = 3;         // ≈сли быстрый ответ, то слово не должно по€витьс€ быстрее, чем через это количество дней


float addDaysMin[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
float addDaysMax[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
float addDaysMinSrc [MAX_RIGHT_REPEATS_GLOBAL_N + 1] = {0, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 1, 1, 1, 2, 2, 3, 3, 4, 1, 5, 1, 5, 1, 7, 1, 2, 9, 1, 3, 11, 1, 3, 14, 1, 3, 8, 17, 1, 3, 9, 20, 1, 3, 9, 23, 1, 3, 11, 27, 1, 3, 14, 35, 1, 3, 10, 18, 42, 1, 3, 10, 20, 50, 1, 3, 10, 25, 60, 1, 3, 10, 20, 35, 70, 1, 3, 10, 20, 40, 80, 1, 3, 10, 20, 45, 90};
float addDaysDiffSrc[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 2, 0, 0, 2, 0, 0, 3, 0, 0, 3, 0, 0, 2, 3, 0, 0, 2, 3, 0, 0, 2, 3, 0, 0, 3, 3, 0, 0, 3, 3, 0, 0, 2, 3, 4, 0, 0, 2, 3, 4, 0, 0, 2, 3, 5, 0, 0, 2, 3, 4, 5, 0, 0, 2, 3, 4, 5, 0, 0, 2, 3, 4, 5};

extern Log logger;

//===============================================================================================
//
//===============================================================================================

LearnWordsApp::LearnWordsApp(): _additionalCheck(this, &_wordsOnDisk), _mandatoryCheck(this, &_wordsOnDisk), _learnNew(this, &_wordsOnDisk), _listening(this, &_wordsOnDisk), _freezedTime(0) 
{
	for (int i = 0; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
	{
		float t = addDaysMinSrc[i];
		if (t >= 1)  
			t -= 0.2f;
		addDaysMin[i] = t;
		addDaysMax[i] = t + addDaysDiffSrc[i];
	}
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::save()
{
	FileOperate::save_to_file(_fullFileName.c_str(), &_wordsOnDisk);
}



//===============================================================================================
// ¬ернуть число переводов в данном слове
//===============================================================================================

int LearnWordsApp::get_translations_num(const char* translation)
{
	const char* p = translation;
	int inBracesNestedCount = 0;
	int commasCount = 0;

	while (*p)
	{
		if (*p == '(')
			++inBracesNestedCount;
		else
			if (*p == ')')
				--inBracesNestedCount;
			else
				if (*p == ',' && inBracesNestedCount == 0)
					++commasCount;
		++p;
	}

	return commasCount + 1;
}

//===============================================================================================
//
//===============================================================================================

bool LearnWordsApp::is_quick_answer(double milliSec, const char* translation)
{
	int translationsNum = get_translations_num(translation);
	int compareWith = QUICK_ANSWER_TIME_MS_FOR_ONE_TRANSLATION; 
	if (translationsNum > 1)
		compareWith = QUICK_ANSWER_TIME_MS_FOR_MORE_TRANSLATIONS;

	return milliSec < compareWith;
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::clear_forgotten()
{
	_forgottenWordsIndices.clear();
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::add_forgotten(int forgottenWordIndex)
{
	_forgottenWordsIndices.push_back(forgottenWordIndex);
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::get_forgotten(std::vector<int>& forgottenWordsIndices)
{
	forgottenWordsIndices = _forgottenWordsIndices;
}


//===============================================================================================
//
//===============================================================================================

bool LearnWordsApp::isWordJustLearnedOrForgotten(const WordsData::WordInfo& w, time_t curTime) const
{
	return (w.rightAnswersNum == 1) &&   // Ёто условие не даЄт попасть в выборку словам высоких уровней, до повторени€ которых осталось несколько часов
		((w.dateOfRepeat >= curTime) && (w.dateOfRepeat <= curTime + addDaysMax[1] * SECONDS_IN_DAY));  // ј это условие не даЄт попасть выборку только что изученным словам на следующий день
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::reset_all_words_to_repeated(int rightAnswersToSet, float minDaysRepeat, float maxDaysRepeat, time_t currentTime)
{
	for (auto& w : _wordsOnDisk._words)
	{
		if (w.rightAnswersNum)
		{
			w.rightAnswersNum = rightAnswersToSet;
			float randDays = rand_float(minDaysRepeat, maxDaysRepeat);
			fill_dates(randDays, w, currentTime);
		}
	}
}


//===============================================================================================
// 
//===============================================================================================

int LearnWordsApp::main_menu_choose_mode(time_t freezedTime)
{
//	_additionalCheck.log_random_test_words(_freezedTime);

	int wordsTimeToRepeatNum = 0;
	int wordsByLevel[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
	recalc_stats(_freezedTime, &wordsTimeToRepeatNum, wordsByLevel);

	int wordsLearnGoodIndex = 0;   // Ќайти индекс, больше которого имеют хорошо изученные слова
	for (int i = 1; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
		if (addDaysMin[i] >= WORDS_LEARNED_GOOD_THRESHOLD)
		{
			wordsLearnGoodIndex = i;
			break;
		}
	int wordsLearnedTotal = 0;
	int wordsLearnedGood = 0;
	for (int i = 1; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
	{
		wordsLearnedTotal += wordsByLevel[i];
		if (i >= wordsLearnGoodIndex)
			wordsLearnedGood += wordsByLevel[i];
	}
	static int prevWordsLearnedGood;
	int deltaWordsLearnedGood = 0;
	if (prevWordsLearnedGood > 0)
		deltaWordsLearnedGood = wordsLearnedGood - prevWordsLearnedGood;
	prevWordsLearnedGood = wordsLearnedGood;

	int mainQueueLen = 0;
	int mainQueueSkipLoopCount = 0;
	int fastQueueLen = 0;
	_additionalCheck.calc_words_for_random_repeat(&mainQueueLen, &mainQueueSkipLoopCount, &fastQueueLen, _freezedTime);

	static int prevSkipLoopCount;
	int deltaSkipLoopCount = 0;
	if (prevSkipLoopCount > 0)
		deltaSkipLoopCount = mainQueueSkipLoopCount - prevSkipLoopCount;
	prevSkipLoopCount = mainQueueSkipLoopCount;

	printf("\n");
	printf("\n");
	printf("1. ¬ыучить новые слова\n");
	printf("2. ≈жедневный повтор  [%d]\n", wordsTimeToRepeatNum);
	printf("3. ѕодучить забытое [%d]\n", (int)_forgottenWordsIndices.size());
	printf("4. јудирование\n");
	printf("5. ѕовторить больше слов\n");
	printf("\n\n");

	printf("¬ыучено слов: %d, из них хорошо: %d (%d)\n", wordsLearnedTotal, wordsLearnedGood, deltaWordsLearnedGood);
	logger("Learned and good: %d, %d, time = %s", wordsLearnedTotal, wordsLearnedGood, get_time_in_text(time(nullptr)));

//	printf("  –андомный повтор: ќсновн=%d (из них skip=%d (%d)), Ѕыстра€=%d ", mainQueueLen, mainQueueSkipLoopCount, deltaSkipLoopCount, fastQueueLen);

	if (!_forgottenWordsIndices.empty())
	{
		for (const auto& index : _forgottenWordsIndices)
		{
			WordsData::WordInfo& w = _wordsOnDisk._words[index];
			printf("==============================================\n%s\n   %s\n", w.word.c_str(), w.translation.c_str());
		}
	}
	else
	{
		auto print_upcoming_words_info = [this, wordsLearnGoodIndex, freezedTime](int stagesBehind)
		{
			for (const auto& w : _wordsOnDisk._words)
			{
				if (w.rightAnswersNum == wordsLearnGoodIndex - stagesBehind)
					printf("%2.1f ", (w.dateOfRepeat - freezedTime) / 3600 / 24.f);
			}
		};

		printf("\n—колько дней до проверки кандидатов на хорошо выученные слова\n");
		printf("-2 этапа: ");
		print_upcoming_words_info(2);
		printf("\n-1 этап: ");
		print_upcoming_words_info(1);
	}
	
	while (true)
	{
		char c = getch_filtered();
		if (c == 27 || c == '1' || c == '2' || c == '3' || c == '4' || c == '5')  // 27 - Esc
			return c;
		//		printf("%d\n", c);
	}

	return 0;
}


//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::recalc_stats(time_t curTime, int* wordsTimeToRepeatNum, int wordsByLevel[])
{
	*wordsTimeToRepeatNum = 0;

	for (int i = 0; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
		wordsByLevel[i] = 0;

	for (const auto& w : _wordsOnDisk._words)
	{
		++(wordsByLevel[w.rightAnswersNum]);

		if (w.dateOfRepeat != 0)
		{
			if (w.rightAnswersNum == 0)
				exit_msg("Semantic error\n");

			if (w.dateOfRepeat < curTime)
				++(*wordsTimeToRepeatNum);
		}
	}

	//	printf("\n%d  - Words that is time to repeat\n\n", wordsTimeToRepeatNum);
}


//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::fill_dates(float randDays, WordsData::WordInfo &w, time_t currentTime)
{
	int secondsPlusCurTime = int(randDays * SECONDS_IN_DAY);
	w.dateOfRepeat = (int)currentTime + secondsPlusCurTime;
	w.cantRandomTestedAfter = (int)currentTime + secondsPlusCurTime / 2;
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::fill_dates_and_save(WordsData::WordInfo& w, time_t currentTime, bool needAdvance_RightAnswersNum, bool isQuickAnswer)
{
	if (needAdvance_RightAnswersNum)
	{
		++w.rightAnswersNum;
		clamp_max(&w.rightAnswersNum, MAX_RIGHT_REPEATS_GLOBAL_N);
	}

	float min = addDaysMin[w.rightAnswersNum];
	float max = addDaysMax[w.rightAnswersNum];

	if (isQuickAnswer)
		min = (min + max) * 0.5f;
	else
		max = (min + max) * 0.5f;

	float randDays = rand_float(min, max);

	if (isQuickAnswer && randDays < MIN_DAYS_IF_QUICK_ANSWER)
	{
		while (w.rightAnswersNum < MAX_RIGHT_REPEATS_GLOBAL_N && addDaysMax[w.rightAnswersNum] < MIN_DAYS_IF_QUICK_ANSWER)
			++w.rightAnswersNum;
		randDays = MIN_DAYS_IF_QUICK_ANSWER;
	}

	fill_dates(randDays, w, currentTime);
	save();
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::print_buttons_hints(const std::string& str, bool needRightKeyHint)
{
	CONSOLE_SCREEN_BUFFER_INFO   csbi;
	csbi.wAttributes = 0;
	bool isColorReadSucsess = false;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole != INVALID_HANDLE_VALUE)
		isColorReadSucsess = GetConsoleScreenBufferInfo(hConsole, &csbi) == TRUE;

	const char* p = str.c_str();
	int bracesNestCount = 0;

	printf("\n");
	if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
		SetConsoleTextAttribute(hConsole, 15);
	while (*p)
	{
		if (*p == '(' || *p == '[')
		{
			++bracesNestCount;
			if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
				SetConsoleTextAttribute(hConsole, 8);
		}

		printf("%c", *p);

		if (*p == ')' || *p == ']')
		{
			--bracesNestCount;
			if (bracesNestCount == 0)
				if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
					SetConsoleTextAttribute(hConsole, 15);
		}

		++p;
	}
	printf("\n");

	if (hConsole != INVALID_HANDLE_VALUE && isColorReadSucsess)
		SetConsoleTextAttribute(hConsole, csbi.wAttributes);

	printf("\n\n  —трелка вверх  - помню хорошо!\n  —трелка вниз   - забыл/перепутал хот€ бы одно значение\n");
	if (needRightKeyHint)
		printf("  —трелка вправо - вспомнил все значени€, но с трудом\n");
}


//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::set_word_as_just_learned(WordsData::WordInfo& w)
{
	w.clear_all();
	w.rightAnswersNum = 1;
}

//===============================================================================================
// 
//===============================================================================================

time_t LearnWordsApp::get_time()
{
//return 1505167116;  // ќтладочный режим со стабильным временем и рандомом. ѕри тесте отвечать на вопросы надо быстро дл€ одинакового результата
	return std::time(nullptr);
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::process(int argc, char* argv[])
{
	std::srand((unsigned)get_time());
	srand((int)get_time());

	if (argc != 3)
	{
		//puts("Ussage:");
		//puts("LearnWords.exe [path to base file]\n");
		//return 0;
		_fullFileName = "C:\\Dimka\\yadisk\\LearnWords\\dima_to_learn.txt";
		_fullRimPath = "C:\\Dimka\\MyLims\\40\\CNN\\CBSBenghaziProblem\\";
	}
	else
	{
		_fullFileName = argv[1];
		_fullRimPath = argv[2];
	}
	FileOperate::load_from_file(_fullFileName.c_str(), &_wordsOnDisk);
	//	wordsOnDisk.export_for_google_doc();

	//FileOperate::save_to_file(_fullFileName.c_str(), &_wordsOnDisk);
	//return;

	while (true)
	{
		clear_console_screen();

		_freezedTime = get_time();   // “екущее врем€ обновл€етс€ один раз перед показом главного меню, чтобы число слов дл€ повтора в меню 
								   // и последующем запуске режима повтора (в нЄм используетс€ запомненный здесь curTime) было одинаковым .
		int keyPressed = main_menu_choose_mode(_freezedTime);
		switch (keyPressed)
		{
		case 27:  // ESC
			printf("%ld\n", int(_freezedTime));
			return;
			break;
		case '1':
			_learnNew.learn_new(_freezedTime, &_additionalCheck);
			break;
		case '2':
			_mandatoryCheck.mandatory_check(_freezedTime, &_additionalCheck, _fullFileName);
			break;
		case '3':
			_learnNew.learn_forgotten(_freezedTime, &_additionalCheck);
			break;
		case '4':
			_listening.listening(_fullRimPath);
			break;
		case '5':
			_additionalCheck.additional_check(_freezedTime, _fullFileName);
			break;
		default:
			break;
		}
	}
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::set_as_forgotten(WordsData::WordInfo& w)
{
	w.rightAnswersNum = std::min(w.rightAnswersNum, DOWN_ANSWERS_FALLBACK);
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::set_as_barely_known(WordsData::WordInfo& w)
{
	w.rightAnswersNum = std::min(w.rightAnswersNum, RIGHT_ANSWERS_FALLBACK);
}


