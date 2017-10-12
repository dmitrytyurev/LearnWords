#include "stdafx.h"
#include "CommonUtility.h"
#include "LearnWordsApp.h"
#include "FileOperate.h"


const int QUICK_ANSWER_TIME_MS = 2900;             // ����� �������� ������ � �������������
const static int MAX_RIGHT_REPEATS_GLOBAL_N = 20;
const static int WORDS_LEARNED_GOOD_THRESHOLD = 14;

float addDaysMin[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 1, 1, 2, 3, 3, 4, 5, 7, 10, 14, 20, 25, 35, 50, 70, 90,  100, 120 };
float addDaysMax[MAX_RIGHT_REPEATS_GLOBAL_N + 1] = { 0, 0.25f, 0.25f, 1, 1, 3, 4, 4, 5, 6, 9, 12, 16, 23, 28, 40, 60, 80, 100, 120, 150 };

void listening(const std::string& rimFolder);

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
	return (w.rightAnswersNum == 1) &&   // ��� ������� �� ��� ������� � ������� ������ ������� �������, �� ���������� ������� �������� ��������� �����
		((w.dateOfRepeat >= curTime) && (w.dateOfRepeat <= curTime + addDaysMax[1] * SECONDS_IN_DAY));  // � ��� ������� �� ��� ������� ������� ������ ��� ��������� ������ �� ��������� ����
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
	_additionalCheck.log_random_test_words(_freezedTime);

	int wordsTimeToRepeatNum = 0;
	int wordsByLevel[MAX_RIGHT_REPEATS_GLOBAL_N + 1];
	recalc_stats(_freezedTime, &wordsTimeToRepeatNum, wordsByLevel);

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
	printf("1. ������� ����� �����\n");
	printf("2. ���������� ������  [%d+~%d]\n", wordsTimeToRepeatNum, int(wordsTimeToRepeatNum * _mandatoryCheck.calc_additional_word_probability(wordsTimeToRepeatNum)));
	printf("3. �������� ������� [%d]\n", (int)_forgottenWordsIndices.size());
	printf("4. �����������\n");
	printf("5. ��������� ������ ����\n");
	printf("\n\n");

	printf("������� ����: %d, �� ��� ������: %d (%d)\n", wordsLearnedTotal, wordsLearnedGood, deltaWordsLearnedGood);
//	printf("  ��������� ������: ������=%d (�� ��� skip=%d (%d)), �������=%d ", mainQueueLen, mainQueueSkipLoopCount, deltaSkipLoopCount, fastQueueLen);

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
	printf("\n%s\n\n\n  ������� �����  - ����� ������!\n  ������� ����   - �����/��������� ���� �� ���� ��������\n", str.c_str());
	if (needRightKeyHint)
		printf("  ������� ������ - �������� ��� ��������, �� � ������\n");
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
//return 1505167116;  // ���������� ����� �� ���������� �������� � ��������. ��� ����� �������� �� ������� ���� ������ ��� ����������� ����������
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
		_fullFileName = "C:\\Dimka\\LearnWords\\dima_to_learn.txt";
		_fullRimPath = "C:\\tmp\\";
	}
	else
	{
		_fullFileName = argv[1];
		_fullRimPath = argv[2];
	}
	FileOperate::load_from_file(_fullFileName.c_str(), &_wordsOnDisk);
	//	wordsOnDisk.export_for_google_doc();

	//wordsOnDisk.reset_all_words_to_repeated(16, 2, 50, time(nullptr)); // ���� ����� �� ���������. ���� ����� ���������� ��� ������ ����� �����, �� ����������� �� ����� ����� ��������� ���� �-����
	//wordsOnDisk.save_to_file();
	//return 0;

	while (true)
	{
		clear_console_screen();

		_freezedTime = get_time();   // ������� ����� ����������� ���� ��� ����� ������� �������� ����, ����� ����� ���� ��� ������� � ���� 
								   // � ����������� ������� ������ ������� (� �� ������������ ����������� ����� curTime) ���� ���������� .
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
			listening(_fullRimPath);
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

void LearnWordsApp::fill_rightAnswersNum(WordsData::WordInfo& w)
{
	++w.rightAnswersNum;
	clamp_max(&w.rightAnswersNum, MAX_RIGHT_REPEATS_GLOBAL_N);

	if (w.cantRandomTestedAfter) // ���� ����� ��������� ����� �������� ��������, �� ��������� ���������� rightAnswersNum �������, ��� �� 1
	{
		int notTestedTimeInterval = int(_freezedTime - w.cantRandomTestedAfter); // ������ ������ ���������, � ������� �������� ����� ����� �� ����������� - �� ������������ ���������, �� ���������
		if (notTestedTimeInterval / SECONDS_IN_DAY > 7)
		{
			int i = 1;
			for (; i <= MAX_RIGHT_REPEATS_GLOBAL_N; ++i)
			{
				if (notTestedTimeInterval <= addDaysMax[i] * SECONDS_IN_DAY)
					break;
			}
			++i;
			clamp_max(&i, MAX_RIGHT_REPEATS_GLOBAL_N);

			w.rightAnswersNum = std::max(w.rightAnswersNum, i);
		}
	}
}
