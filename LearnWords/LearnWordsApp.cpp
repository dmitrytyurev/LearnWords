#include "stdafx.h"
#include "CommonUtility.h"
#include "LearnWordsApp.h"
#include "FileOperate.h"


const int QUICK_ANSWER_TIME_MS = 2900;             // Время быстрого ответа в миллисекундах
const static int MAX_RIGHT_REPEATS_GLOBAL_N = 20;
const static int WORDS_LEARNED_GOOD_THRESHOLD = 14;

float addDaysMin[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 1, 1, 2, 3, 3, 4, 5, 7, 10, 14, 20, 25, 35, 50, 70, 90,  100, 120 };
float addDaysMax[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 1, 1, 3, 4, 4, 5, 6, 9, 12, 16, 23, 28, 40, 60, 80, 100, 120, 150 };

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::save()
{
   // использовать  std::string _fullFileName;
}


//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::clear_forgotten()
{
	forgottenWordsIndices.clear();
}

//===============================================================================================
//
//===============================================================================================

bool LearnWordsApp::is_quick_answer(double milliSec)
{
	return milliSec < QUICK_ANSWER_TIME_MS;
}


//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::add_forgotten(int forgottenWordIndex)
{
	forgottenWordsIndices.push_back(forgottenWordIndex);
}

//===============================================================================================
//
//===============================================================================================

bool LearnWordsApp::isWordJustLearnedOrForgotten(const WordsData::WordInfo& w, time_t curTime) const
{
	return (w.rightAnswersNum == 1) &&   // Это условие не даёт попасть в выборку словам высоких уровней, до повторения которых осталось несколько часов
		((w.dateOfRepeat >= curTime) && (w.dateOfRepeat <= curTime + addDaysMax[1] * SECONDS_IN_DAY));  // А это условие не даёт попасть выборку только что изученным словам на следующий день
}

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::reset_all_words_to_repeated(int rightAnswersToSet, float minDaysRepeat, float maxDaysRepeat, time_t currentTime)
{
	for (auto& w : wordsOnDisk._words)
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

int LearnWordsApp::main_menu_choose_mode()
{
	_additionalCheck.log_random_test_words();

	int wordsTimeToRepeatNum = 0;
	int wordsJustLearnedAndForgottenNum = 0;
	int wordsByLevel[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
	recalc_stats(curTime, &wordsTimeToRepeatNum, &wordsJustLearnedAndForgottenNum, wordsByLevel);

	int wordsLearnedTotal = 0;
	int wordsLearnedGood = 0;
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
	_additionalCheck.calc_words_for_random_repeat(&mainQueueLen, &mainQueueSkipLoopCount, &fastQueueLen);

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
	printf("4. Обязательная проверка  [%d+~%d]\n", wordsTimeToRepeatNum, int(wordsTimeToRepeatNum * _mandatoryCheck.calc_additional_word_probability(wordsTimeToRepeatNum)));
	printf("\n\n");

	for (const auto& index : forgottenWordsIndices)
	{
		WordsData::WordInfo& w = wordsOnDisk._words[index];
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

void LearnWordsApp::recalc_stats(time_t curTime, int* wordsTimeToRepeatNum, int* wordsJustLearnedAndForgottenNum, int wordsByLevel[])
{
	*wordsTimeToRepeatNum = 0;
	*wordsJustLearnedAndForgottenNum = 0;

	for (int i = 0; i < MAX_RIGHT_REPEATS_GLOBAL_N + 1; ++i)
		wordsByLevel[i] = 0;

	for (const auto& w : wordsOnDisk._words)
	{
		++(wordsByLevel[w.rightAnswersNum]);

		if (w.dateOfRepeat != 0)
		{
			if (w.rightAnswersNum == 0)
				exit_msg("Semantic error\n");

			if (w.dateOfRepeat < curTime)
				++(*wordsTimeToRepeatNum);

			if (isWordJustLearnedOrForgotten(w, curTime))
				++(*wordsJustLearnedAndForgottenNum);
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

void LearnWordsApp::fill_dates_and_save(WordsData::WordInfo& w, time_t currentTime, LearnWordsApp::RandScopePart randScopePart)
{
	float min = addDaysMin[w.rightAnswersNum];
	float max = addDaysMax[w.rightAnswersNum];

	if (randScopePart == LearnWordsApp::RandScopePart::LOWER_PART)
		max = (min + max) * 0.5f;
	else
		if (randScopePart == LearnWordsApp::RandScopePart::HI_PART)
			min = (min + max) * 0.5f;

	float randDays = rand_float(min, max);

	fill_dates(randDays, w, currentTime);
	save();
}

//===============================================================================================
// 
//===============================================================================================

void LearnWordsApp::print_buttons_hints(const std::string& str, bool needRightKeyHint)
{
	printf("\n%s\n\n\n  Стрелка вверх  - помню хорошо!\n  Стрелка вниз   - забыл/перепутал хотя бы одно значение\n", str.c_str());
	if (needRightKeyHint)
		printf("  Стрелка вправо - вспомнил все значения, но с трудом\n");
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

void LearnWordsApp::process(int argc, char* argv[])
{
	if (argc != 2)
	{
		//puts("Ussage:");
		//puts("LearnWords.exe [path to base file]\n");
		//return 0;
		FileOperate::load_from_file("C:\\Dimka\\LearnWords\\dima_to_learn.txt", &wordsOnDisk);
	}
	else
		FileOperate::load_from_file(argv[1], &wordsOnDisk);
	//	wordsOnDisk.export_for_google_doc();

	//wordsOnDisk.reset_all_words_to_repeated(16, 2, 50, time(nullptr)); // Если давно не занимался. Если перед коррекцией уже выучил новые слова, то скопировать их назад после обработки этой ф-цией
	//wordsOnDisk.save_to_file();
	//return 0;

	while (true)
	{
		clear_console_screen();
		curTime = time(nullptr);   // Текущее время обновляется один раз перед показом главного меню, чтобы число слов для повтора в меню 
								   // и последующем запуске режима повтора (в нём используется запомненный здесь curTime) было одинаковым .
		int keyPressed = main_menu_choose_mode();
		switch (keyPressed)
		{
		case 27:  // ESC
			printf("%ld\n", int(curTime));
			return;
			break;
		case '1':
			_learnNew.learn_new(curTime, &_additionalCheck);
			break;
		case '2':
			_repeatOfRecent.repeat_of_recent(curTime);
			break;
		case '3':
			_additionalCheck.additional_check(curTime, _fullFileName);
			break;
		case '4':
			_mandatoryCheck.mandatory_check(curTime, &_additionalCheck, MAX_RIGHT_REPEATS_GLOBAL_N, _fullFileName);
			break;
		default:
			break;
		}
	}
}

