#include "stdafx.h"
#include "CommonUtility.h"
#include "LearnWordsApp.h"
#include "FileOperate.h"


const int QUICK_ANSWER_TIME_MS = 2900;             // ¬рем€ быстрого ответа в миллисекундах
const int MAX_RIGHT_REPEATS_GLOBAL_N = 38;
const int WORDS_LEARNED_GOOD_THRESHOLD = 25; // „исло дней в addDaysMin, по которому выбираетс€ индекс, чтобы считать слова хорошо изученными
const int CAN_SKIP_IF_LESS_DAYS = 10;  // ѕри быстром правильном ответе слову можно сделать дополнительный инкремент, если его следующий повтор планировалс€ не более, чем через это количество дней

float addDaysMin[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 0.8f, 0.8f, 2, 2, 3, 4, 0.8f, 4, 5, 7, 0.8f, 7, 0.8f, 10, 0.8f, 14, 0.8f, 14, 0.8f, 20, 0.8f, 25, 0.8f, 25, 0.8f, 35, 0.8f, 35, 0.8f, 40, 0.8f, 50, 0.8f, 70, 0.8f, 90 };
float addDaysMax[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 0.8f, 0.8f, 3, 3, 4, 5, 0.8f, 5, 6, 9, 0.8f, 9, 0.8f, 12, 0.8f, 16, 0.8f, 16, 0.8f, 23, 0.8f, 28, 0.8f, 28, 0.8f, 40, 0.8f, 40, 0.8f, 50, 0.8f, 60, 0.8f, 80, 0.8f, 100};

extern Log logger;

//===============================================================================================
//
//===============================================================================================

void LearnWordsApp::save()
{
	FileOperate::save_to_file(_fullFileName.c_str(), &_wordsOnDisk);
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

int LearnWordsApp::main_menu_choose_mode()
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
	printf("2. ≈жедневный повтор  [%d+~%d]\n", wordsTimeToRepeatNum, int(wordsTimeToRepeatNum * _mandatoryCheck.calc_additional_word_probability(wordsTimeToRepeatNum)));
	printf("3. ѕодучить забытое [%d]\n", (int)_forgottenWordsIndices.size());
	printf("4. јудирование\n");
	printf("5. ѕовторить больше слов\n");
	printf("\n\n");

	printf("¬ыучено слов: %d, из них хорошо: %d (%d)\n", wordsLearnedTotal, wordsLearnedGood, deltaWordsLearnedGood);
	logger("Learned and good: %d, %d, time = %s", wordsLearnedTotal, wordsLearnedGood, get_time_in_text(time(nullptr)));

//	printf("  –андомный повтор: ќсновн=%d (из них skip=%d (%d)), Ѕыстра€=%d ", mainQueueLen, mainQueueSkipLoopCount, deltaSkipLoopCount, fastQueueLen);

	for (const auto& index : _forgottenWordsIndices)
	{
		WordsData::WordInfo& w = _wordsOnDisk._words[index];
		printf("==============================================\n%s\n   %s\n", w.word.c_str(), w.translation.c_str());
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
	printf("\n%s\n\n\n  —трелка вверх  - помню хорошо!\n  —трелка вниз   - забыл/перепутал хот€ бы одно значение\n", str.c_str());
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

	//wordsOnDisk.reset_all_words_to_repeated(16, 2, 50, time(nullptr)); // ≈сли давно не занималс€. ≈сли перед коррекцией уже выучил новые слова, то скопировать их назад после обработки этой ф-цией
	//wordsOnDisk.save_to_file();
	//return 0;

	while (true)
	{
		clear_console_screen();

		_freezedTime = get_time();   // “екущее врем€ обновл€етс€ один раз перед показом главного меню, чтобы число слов дл€ повтора в меню 
								   // и последующем запуске режима повтора (в нЄм используетс€ запомненный здесь curTime) было одинаковым .
		int keyPressed = main_menu_choose_mode();
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

void LearnWordsApp::fill_rightAnswersNum(WordsData::WordInfo& w, bool isQuickAnswer)
{
	// ≈сли слово проверили после длинного перерыва, то попробуем продвинуть rightAnswersNum перед тем, как будет сделан его инкремент
	if (w.cantRandomTestedAfter && addDaysMin[w.rightAnswersNum] > 5)
	{
		int notTestedTimeInterval = int(_freezedTime - w.cantRandomTestedAfter); // ќценка снизу интервала, в течении которого слово точно не провер€лось - ни об€зательной проверкой, ни случайной
		int i = MAX_RIGHT_REPEATS_GLOBAL_N;
		while (true)
		{
			if (addDaysMin[i] > 5 && notTestedTimeInterval > (addDaysMin[i] + 2) * SECONDS_IN_DAY)
			{
				w.rightAnswersNum = i;
				break;
			}
			if (--i <= w.rightAnswersNum)
				break;
		}
	}

	++w.rightAnswersNum;
	clamp_max(&w.rightAnswersNum, MAX_RIGHT_REPEATS_GLOBAL_N);
//	if (isQuickAnswer && addDaysMax[w.rightAnswersNum] < CAN_SKIP_IF_LESS_DAYS)
//	{
//		++w.rightAnswersNum;
//		clamp_max(&w.rightAnswersNum, MAX_RIGHT_REPEATS_GLOBAL_N);
//	}
}

//void LearnWordsApp::test()
//{
//	const int N_NUM = 3;
//	int nVals[N_NUM] = {12, 14, 16};
//
//	const int TIME_NUM = 2;
//	int timeVals[TIME_NUM] = {400, 600};
//
//	for (int ni = 0; ni<N_NUM; ++ni)
//		for (int timi = 0; timi < TIME_NUM; ++timi)
//		{
//			WordsData::WordInfo w;
//			w.rightAnswersNum = nVals[ni];
//			w.cantRandomTestedAfter = _freezedTime - timeVals[timi] * 3600;
//			fill_rightAnswersNum_new(w);
//			printf("time=%d, n=%d nNew=%d\n", timeVals[timi], nVals[ni], w.rightAnswersNum);
//		}
//	exit(1);
//}

