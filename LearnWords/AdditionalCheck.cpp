#include "stdafx.h"
#include "AdditionalCheck.h"
#include <algorithm>
#include <chrono>

#include "LearnWordsApp.h"
#include "CommonUtility.h"
#include "CloseTranslationWordsManager.h"
#include "FileOperate.h"


const int REPEAT_AFTER_N_DAYS = 2;    // ����� ��������� � ������� ������� ���������� �������, ����� ������ ��������� ����� ��� ����� �����
extern Log logger;


//===============================================================================================
// 
//===============================================================================================

int AdditionalCheck::calc_max_randomTestIncID(bool isFromFastQueue)
{
	int max = -1;
	for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)
	{
		WordsData::WordInfo& w = _pWordsData->_words[i];

		if (w.isInFastRandomQueue == isFromFastQueue)
			max = std::max(max, _pWordsData->_words[i].randomTestIncID);
	}
	return max;
}


//===============================================================================================
// 
//===============================================================================================

void AdditionalCheck::fill_indices_of_random_repeat_words(std::vector<int> &indicesOfWords, bool isFromFastQueue, time_t freezedTime)
{
	indicesOfWords.clear();
	for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)
	{
		WordsData::WordInfo& w = _pWordsData->_words[i];

		if (can_random_tested(w, freezedTime) && w.isInFastRandomQueue == isFromFastQueue)
			indicesOfWords.push_back(i);
	}

	WordsData* pWordsData = _pWordsData;

	std::sort(indicesOfWords.begin(), indicesOfWords.end(), [pWordsData](int i, int j) {
		return pWordsData->_words[i].randomTestIncID < pWordsData->_words[j].randomTestIncID; });
}

//===============================================================================================
// 
//===============================================================================================

void AdditionalCheck::calc_words_for_random_repeat(int* mainQueueLen, int* mainQueueSkipLoopCount, int* fastQueueLen, time_t freezedTime)
{
	std::vector<int> indicesOfWords;
	fill_indices_of_random_repeat_words(indicesOfWords, false, freezedTime);
	*mainQueueLen = indicesOfWords.size();

	*mainQueueSkipLoopCount = 0;
	for (const auto& index : indicesOfWords)
	{
		WordsData::WordInfo& w = _pWordsData->_words[index];
		if (w.isNeedSkipOneRandomLoop)
			++(*mainQueueSkipLoopCount);
	}

	fill_indices_of_random_repeat_words(indicesOfWords, true, freezedTime);
	*fastQueueLen = indicesOfWords.size();
}

//===============================================================================================
// 
//===============================================================================================

void AdditionalCheck::log_random_test_words(time_t freezedTime)
{
	std::vector<int> indicesOfWords;   // ������� ���������� ��� �������� ����
	fill_indices_of_random_repeat_words(indicesOfWords, false, freezedTime); // �������� indicesOfWords
	logger("=== Words to random repeat = %d\n", indicesOfWords.size());
	for (const auto& index : indicesOfWords)
	{
		WordsData::WordInfo& w = _pWordsData->_words[index];
		logger("  %s %d skip=%d\n", w.word.c_str(), w.randomTestIncID, w.isNeedSkipOneRandomLoop);
	}

	indicesOfWords.clear();
	fill_indices_of_random_repeat_words(indicesOfWords, true, freezedTime);
	logger("=== Fast words to random repeat = %d\n", indicesOfWords.size());
	for (const auto& index : indicesOfWords)
	{
		WordsData::WordInfo& w = _pWordsData->_words[index];
		logger("  %s %d\n", w.word.c_str(), w.randomTestIncID);
	}
}

//===============================================================================================
// 
//===============================================================================================

int AdditionalCheck::get_word_to_repeat_inner(time_t freezedTime)
{
	std::vector<int> indicesOfWordsCommon;   // ������� ���������� ��� �������� ���� �� ����� �������
	std::vector<int> indicesOfWordsFast;     // ������� ���������� ��� �������� ���� �� ������� �������

	fill_indices_of_random_repeat_words(indicesOfWordsCommon, false, freezedTime);  // �������� indicesOfWordsCommon
	fill_indices_of_random_repeat_words(indicesOfWordsFast, true, freezedTime);   // �������� indicesOfWordsTrue

	if ((rand_float(0, 1) < 0.5f || indicesOfWordsCommon.empty()) && !indicesOfWordsFast.empty())
	{
		return indicesOfWordsFast[0];
	}
	else
	{
		if (indicesOfWordsCommon.empty())
			return -1;
		else
			return indicesOfWordsCommon[rand_int(0, std::min(0u, indicesOfWordsCommon.size() - 1))];  // FIXME!!! 4u
	}
}


//===============================================================================================
// 
//===============================================================================================

int AdditionalCheck::get_word_to_repeat(time_t freezedTime)
{
	while (true)
	{
		int index = get_word_to_repeat_inner(freezedTime);
		if (index == -1)
			return index;

		WordsData::WordInfo& w = _pWordsData->_words[index];
		if (w.isNeedSkipOneRandomLoop == false)
			return index;

		put_word_to_end_of_random_repeat_queue_common(w);
	}
}

//===============================================================================================
// 
//===============================================================================================

void AdditionalCheck::put_word_to_end_of_random_repeat_queue_common(WordsData::WordInfo& w)
{
	w.randomTestIncID = calc_max_randomTestIncID(false) + 1;
	logger("put rand common = %d\n", w.randomTestIncID);
	w.isInFastRandomQueue = false;
	w.isNeedSkipOneRandomLoop = false;
	w.cantRandomTestedBefore = 0;
}

//===============================================================================================
// 
//===============================================================================================

void AdditionalCheck::put_word_to_end_of_random_repeat_queue_fast(WordsData::WordInfo& w, time_t currentTime)
{
	w.randomTestIncID = calc_max_randomTestIncID(true) + 1;
	logger("put rand fast = %d\n", w.randomTestIncID);
	w.isInFastRandomQueue = true;
	w.isNeedSkipOneRandomLoop = false;
	w.cantRandomTestedBefore = (int)currentTime + int(REPEAT_AFTER_N_DAYS * SECONDS_IN_DAY);;
}

//===============================================================================================
// 
//===============================================================================================

bool AdditionalCheck::can_random_tested(WordsData::WordInfo& w, time_t freezedTime) const
{
	if (w.rightAnswersNum == 0)
		return false;  // ����� ������ ������������ � ��������� ������� ������, ��� ��� ��� �� �����

	if (w.cantRandomTestedAfter != 0 && freezedTime >= w.cantRandomTestedAfter)
		return false;  // ����� ������ ������������ � ��������� ������� ������, ��� �������� ������ �������� ������� �� �������� ��� ������

	if (w.cantRandomTestedBefore != 0 && freezedTime <= w.cantRandomTestedBefore)
		return false;  // ����� ������ ������������ � ��������� ������� ��������� �� ������� ��� ��������� � ����� ������� ������� ����� ��� ����� ��������

	if (_learnWordsApp->isWordJustLearnedOrForgotten(w, freezedTime))
		return false; // ����� ������ ������������ � ��������� ������� ������, ��� ��� ������ ��������� � ������ ������ ��� ��������� ��� ������� ����. 

	return true;
}

//===============================================================================================
// 
//===============================================================================================

void AdditionalCheck::additional_check(time_t freezedTime, const std::string& fullFileName)
{
	_learnWordsApp->clear_forgotten();
	clear_console_screen();
	printf("\n������� ��������� ���� ������ ���������: ");
	int wordsToRepeatNum = enter_number_from_console();
	if (wordsToRepeatNum == 0)
		return;

	// ������� ���� �������� ����

	for (int i = 0; i < wordsToRepeatNum; ++i)
	{
		int wordToRepeatIndex = get_word_to_repeat(freezedTime);
		if (wordToRepeatIndex == -1)
			break;
		WordsData::WordInfo& w = _pWordsData->_words[wordToRepeatIndex];

		clear_console_screen();
		printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
		auto t_start = std::chrono::high_resolution_clock::now();
		logger("Random repeat, word = %s, === %s, time = %s", w.word.c_str(), fullFileName.c_str(), get_time_in_text(freezedTime));
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');

		auto t_end = std::chrono::high_resolution_clock::now();
		bool isQuickAnswer = _learnWordsApp->is_quick_answer(std::chrono::duration<double, std::milli>(t_end - t_start).count());

		while (true)
		{
			clear_console_screen();
			printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
			_learnWordsApp->print_buttons_hints(w.translation, true);
			printf("\n  ��������: %d, ������� ����� = %d\n", wordsToRepeatNum - i - 1, int(isQuickAnswer));
			CloseTranslationWordsManager ctwm(_learnWordsApp, _pWordsData, wordToRepeatIndex);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // ������� ����� (������ ����� ��������)
			{
				put_word_to_end_of_random_repeat_queue_common(w);
				if (isQuickAnswer)
					w.isNeedSkipOneRandomLoop = true;
				_learnWordsApp->save();
				break;
			}
			else
				if (c == 80) // ������� ���� (������ �����)
				{
					_learnWordsApp->add_forgotten(wordToRepeatIndex);
					_learnWordsApp->set_word_as_just_learned(w);
					_learnWordsApp->fill_dates_and_save(w, freezedTime, LearnWordsApp::RandScopePart::ALL);
					break;
				}
				else
					if (c == 77) // ������� ������ (������ ����� �� ����� ��������)
					{
						_learnWordsApp->add_forgotten(wordToRepeatIndex);
						put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
						_learnWordsApp->save();
						break;
					}
		}
	}
}

