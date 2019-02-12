#include "stdafx.h"
#include <vector>
#include <chrono>
#include <algorithm>

#include "LearnWordsApp.h"
#include "AdditionalCheck.h"
#include "CommonUtility.h"
#include "LearnNew.h"

const int TIMES_TO_GUESS_TO_LEARNED = 3;  // ������� ��� ����� ��������� ������� �������� �����, ����� ��� ��������� �������� ���������
const int TIMES_TO_REPEAT_TO_LEARN  = 2;  // ������� ��� ��� �������� �������� ��� ����� ����� � ���������, ������ ��� ������ ���������� ��� ��������
const int TIMES_TO_SHOW_A_WORD      = 5;  // ������� ����� �������� ������� ����� ��� ��������� ��������

//===============================================================================================
// 
//===============================================================================================

DistractWordsSupplier::DistractWordsSupplier(LearnWordsApp* learnWordsApp, time_t freezedTime) : _learnWordsApp(learnWordsApp)
{
	// �������� ������ ����, ������� ����� ���������� � �������� ������� ��� ����������. �� ������� ��� ���������� �����, ������� ���� ������� ���������:
	// ��� ����� ������� ���� ��������� ��� ����� ����� ����, ����� ��� �����, �� ������� �� ������� �������� ���������, �� ����� ������

	std::vector<WordToCheck> wordsToMandatoryCheck;
	_learnWordsApp->collect_words_to_mandatory_check(wordsToMandatoryCheck, freezedTime);

	std::vector<WordRememberedLong> toSort;
	for (const auto& v: learnWordsApp->_wordRememberedLong)
		toSort.push_back(v);
	std::sort(toSort.begin(), toSort.end(), [](const auto& a, const auto& b) { return a.durationOfRemember > b.durationOfRemember; }); // ��������� �� ���������� ������� �������

	for (const auto& w : toSort)
	{
		distractWords.push_back(DistractWord(w.index, FromWhatSource::FROM_REMEMBERED_LONG));
		if (!wordsToMandatoryCheck.empty())
		{
			WordToCheck w = wordsToMandatoryCheck.front();
			wordsToMandatoryCheck.erase(wordsToMandatoryCheck.begin());
			distractWords.push_back(DistractWord(w._index, FromWhatSource::FROM_MANDATORY));
		}
	}
	for (const auto& w : wordsToMandatoryCheck)
		distractWords.push_back(DistractWord(w._index, FromWhatSource::FROM_MANDATORY));
}

//===============================================================================================
// 
//===============================================================================================

DistractWord DistractWordsSupplier::get_word(time_t freezedTime, AdditionalCheck* pAdditionalCheck)
{
	if (returnWordsFromAdditional > 0 || distractWords.empty())
	{
		--returnWordsFromAdditional;
		int indexAdditional = pAdditionalCheck->get_word_to_repeat(freezedTime);
		if (indexAdditional == -1)
		{
			returnWordsFromAdditional = 0;
			return DistractWord(-1, FromWhatSource::NOT_INITIALIZED);
		}
		return DistractWord(indexAdditional, FromWhatSource::FROM_ADDITIONAL);
	}
	else
	{
		DistractWord w = distractWords[index];
		++index;
		if (index == distractWords.size())
		{
			index = 0;
			isFirstCycle = false;
			returnWordsFromAdditional = 50 - distractWords.size();
			clamp_min(&returnWordsFromAdditional, 0);
		}
		return w;
	}
}

//===============================================================================================
// 
//===============================================================================================

bool DistractWordsSupplier::is_first_cycle()
{
	return isFirstCycle;
}

//===============================================================================================
// 
//===============================================================================================

void LearnNew::print_masked_translation(const char* _str, int symbolsToShowNum)
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

		if (isInWord && (charCounterInWord > symbolsToShowNum))
			printf("_");
		else
			printf("%c", *str);
		++str;
	}
}

//===============================================================================================
// ��������� �������� � ��������� ��� ������������� ������� 
//===============================================================================================

void LearnNew::put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, bool needRandomInsert)
{
	int pos = (int)queue.size();

	if (needRandomInsert && (pos > 0 && rand_float(0, 1) < 0.2f))
		--pos;

	queue.insert(queue.begin() + pos, wordToPut);
}

//===============================================================================================
// 
//===============================================================================================

bool LearnNew::are_all_words_learned(std::vector<WordToLearn>& queue)
{
	for (const auto& word : queue)
		if (word._localRightAnswersNum < TIMES_TO_GUESS_TO_LEARNED)
			return false;
	return true;
}

//===============================================================================================
// 
//===============================================================================================

void LearnNew::learn_new(time_t freezedTime, AdditionalCheck* pAdditionalCheck)
{
	_learnWordsApp->clear_forgotten();
	std::vector<int> wordsToLearnIndices;

	// �������� ������ �������� ����, ������� ����� �����

	clear_console_screen();
	printf("\n������� ���� ������ �������: ");
	int additionalWordsToLearn = enter_number_from_console();

	if (additionalWordsToLearn > 0)
	{
		for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)
		{
			const WordsData::WordInfo& w = _pWordsData->_words[i];
			if (w.rightAnswersNum == 0)
			{
				wordsToLearnIndices.push_back(i);
				if (--additionalWordsToLearn == 0)
					break;
			}
		}
	}

	if (wordsToLearnIndices.empty())
		return;

	// ��������� �������� (���������� ��� ����� �� ������ ����)

	for (const auto& index : wordsToLearnIndices)
	{
		const WordsData::WordInfo& w = _pWordsData->_words[index];
		clear_console_screen();
		printf("\n%s\n\n", w.word.c_str());
		printf("%s", w.translation.c_str());
		printf("\n");
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
	}

	// ��������� �������� (���������� �������� ���, ������� ������� �����, �� ���������� �����������)

	for (int i2 = 0; i2 < TIMES_TO_REPEAT_TO_LEARN; ++i2)
	{
		for (const auto& index : wordsToLearnIndices)
		{
			const WordsData::WordInfo& w = _pWordsData->_words[index];
			for (int i3 = 0; i3 < TIMES_TO_SHOW_A_WORD; ++i3)
			{
				int symbolsToShowNum = (i3 != TIMES_TO_SHOW_A_WORD -1 ? i3 : 100);

				clear_console_screen();
				printf("\n%s\n\n", w.word.c_str());
				print_masked_translation(w.translation.c_str(), symbolsToShowNum);
				printf("\n\n  ������� ������ - ��������� �� ����� �����\n  ������ -         �������� �� �����");

				char c = 0;
				do
				{
					c = getch_filtered();
					if (c == 27)
						return;
					if (c == ' ')  // ������ - �������� ����� �������
					{
						if (i3 < TIMES_TO_SHOW_A_WORD - 2)
							i3 = TIMES_TO_SHOW_A_WORD - 2;  // ����� ������� �����, ��� �������� ���������
						break;
					}
				} while (c != 77);  // ������� ������ - ��������� �� ������ �������
			}
		}
	}

	// ������ ���� - ����� ������������ ��� ��������. ���� ������������ ������� �������� ����� TIMES_TO_GUESS_TO_LEARNED ���,
	// �� ����� ��������� ���������. ���� �������� �������������, ����� ��� ����� �������.

	std::vector<WordToLearn> learnCycleQueue;  // ����������� ������� ���� � �������� �������� (��������� � �����, ���� �� ������)

											   // ������� �����, ������� ����� ������� � �������
	for (const auto& index : wordsToLearnIndices)
	{
		WordToLearn word(index, FromWhatSource::FROM_LEANRING_QUEUE);
		learnCycleQueue.push_back(word);
	}

	const float treshold_min = 0.4f;
	const float treshold_max = 0.6f;
	float treshold = interp_clip(10.f, treshold_min, 5.f, treshold_max, (float)wordsToLearnIndices.size());  // ��������� ����������������. ���� ���� ������ ����, �� �� ������ �������� ������ ����

																											 // ������� ���� ��������
	while (true)
	{
		clear_console_screen();

		// ������� �����, ������� ����� ����������
		WordToLearn wordToLearn;

		int wordToRepeatIndex = pAdditionalCheck->get_word_to_repeat(freezedTime);

		if (rand_float(0, 1) > treshold || wordToRepeatIndex == -1)
		{
			wordToLearn = learnCycleQueue[0];
			learnCycleQueue.erase(learnCycleQueue.begin());
		}
		else
		{
			wordToLearn = WordToLearn(wordToRepeatIndex, FromWhatSource::FROM_ADDITIONAL);
		}

		// ���������� �����
		WordsData::WordInfo& w = _pWordsData->_words[wordToLearn._index];
		printf("\n%s\n", w.word.c_str());
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
		_learnWordsApp->print_buttons_hints(w.translation, false);

		// ������������ ����� - ����� �� ������������ �����
		while (true)
		{
			c = getch_filtered();
			if (c == 27)  // ESC
				return;
			if (c == 72)  // ������� �����
			{
				switch (wordToLearn._fromWhatSource)
				{
				case FromWhatSource::FROM_LEANRING_QUEUE:
					if (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED)
					{
						_learnWordsApp->set_word_as_just_learned(w);
						_learnWordsApp->fill_dates_and_save(w, freezedTime, false, false);
						if (are_all_words_learned(learnCycleQueue))
							return;
					}
					put_to_queue(learnCycleQueue, wordToLearn, true);
					break;
				case FromWhatSource::FROM_ADDITIONAL:
					pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
					_learnWordsApp->save();
					break;
				}
				break;
			}
			else
				if (c == 80) // ������� ����
				{
					switch (wordToLearn._fromWhatSource)
					{
					case FromWhatSource::FROM_LEANRING_QUEUE:
						wordToLearn._localRightAnswersNum = 0;
						put_to_queue(learnCycleQueue, wordToLearn, true);
						w.clear_all();
						_learnWordsApp->save();
						break;
					case FromWhatSource::FROM_ADDITIONAL:
						pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
						_learnWordsApp->add_forgotten(wordToRepeatIndex);
						_learnWordsApp->set_as_forgotten(w);
						_learnWordsApp->fill_dates_and_save(w, freezedTime, false, false);
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
extern Log logger;

void LearnNew::learn_forgotten(time_t freezedTime, AdditionalCheck* pAdditionalCheck)
{
	std::vector<int> wordsToLearnIndices;
	_learnWordsApp->get_forgotten(wordsToLearnIndices);  // ������� ������ ������� ���� �� ��������
	if (wordsToLearnIndices.empty())
		return;
	_learnWordsApp->clear_forgotten();

	// ��������� �������� (���������� ��� ����� �� ������ ����)

	for (const auto& index : wordsToLearnIndices)
	{
		const WordsData::WordInfo& w = _pWordsData->_words[index];
		clear_console_screen();
		printf("\n%s\n\n", w.word.c_str());
		printf("%s", w.translation.c_str());
		printf("\n");
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');
	}

	// ������ ���� - ����� ������������ ��� ��������. ���� ������������ ������� �������� ����� TIMES_TO_GUESS_TO_LEARNED ���,
	// �� ����� ��������� ���������. ���� �������� �������������, ����� ��� ����� �������.

	DistractWordsSupplier distractWordsSupplier(_learnWordsApp, freezedTime); // ���������� ��������� ����������� ����

	std::vector<WordToLearn> learnCycleQueue;  // ����������� ������� ���� � �������� ����������� (��������� � �����, ���� �� ������)

	for (const auto& index : wordsToLearnIndices) // ������� �����, ������� ����� ���������� � �������
		learnCycleQueue.push_back(WordToLearn(index, FromWhatSource::FROM_LEANRING_QUEUE));

	const int minRepeatFrom = 7;
	int showFromRandomNum = rand_int(minRepeatFrom, minRepeatFrom + 2);
	while (true)
	{
		clear_console_screen();

		// ������� �����, ������� ����� ����������
		WordToLearn wordToLearn;

		--showFromRandomNum;
		if (showFromRandomNum > -1)
		{
			DistractWord distractWord = distractWordsSupplier.get_word(freezedTime, pAdditionalCheck);
			if (distractWord.index != -1)
				wordToLearn = WordToLearn(distractWord.index, distractWord.fromWhatSource);
		}
		if (wordToLearn._fromWhatSource == FromWhatSource::NOT_INITIALIZED)
		{
			int curRepeatFrom = minRepeatFrom / wordsToLearnIndices.size();
			showFromRandomNum = rand_int(curRepeatFrom, curRepeatFrom + 1);
			wordToLearn = learnCycleQueue[0];
			learnCycleQueue.erase(learnCycleQueue.begin());
		}

		// ������� � ���������� �������� � �������
		int sumRightUnswers = 0;
		if (wordToLearn._fromWhatSource == FromWhatSource::FROM_LEANRING_QUEUE)
			sumRightUnswers += wordToLearn._localRightAnswersNum;
		for (const auto& word : learnCycleQueue)
			sumRightUnswers += word._localRightAnswersNum;
		int progress = sumRightUnswers * 100 / (wordsToLearnIndices.size() * TIMES_TO_GUESS_TO_LEARNED);
		printf("\nProgress = %d\n", progress);

		// ���������� �����
		WordsData::WordInfo& w = _pWordsData->_words[wordToLearn._index];
		printf("\n%s\n", w.word.c_str());

		auto t_start = std::chrono::high_resolution_clock::now();
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');

		auto t_end = std::chrono::high_resolution_clock::now();
		double durationForAnswer = std::chrono::duration<double, std::milli>(t_end - t_start).count();
		bool ifTooLongAnswer = false;
		double extraDurationForAnswer = 0;
		bool isQuickAnswer = _learnWordsApp->is_quick_answer(durationForAnswer, w.translation.c_str(), &ifTooLongAnswer, &extraDurationForAnswer);

		_learnWordsApp->print_buttons_hints(w.translation, false);

		// ������������ ����� - ����� �� ������������ �����
		while (true)
		{
			c = getch_filtered();
			if (c == 27)  // ESC
				return;

			if (c == 72)  // ������� �����
			{
				if (wordToLearn._fromWhatSource == FromWhatSource::FROM_LEANRING_QUEUE)
				{
					if (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED)
					{
						if (are_all_words_learned(learnCycleQueue))
							return;
					}
					put_to_queue(learnCycleQueue, wordToLearn, false);
				}
				else
				{
					if (wordToLearn._fromWhatSource == FromWhatSource::FROM_MANDATORY && distractWordsSupplier.is_first_cycle())
					{
						pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
						if (isQuickAnswer)
							w.isNeedSkipOneRandomLoop = true;
						_learnWordsApp->fill_dates_and_save(w, freezedTime, true, isQuickAnswer);
					}
					_learnWordsApp->update_time_remembered_long(wordToLearn._index, extraDurationForAnswer);
					if (ifTooLongAnswer && !_learnWordsApp->is_in_forgotten(wordToLearn._index))
						_learnWordsApp->_wordRememberedLong.insert(WordRememberedLong(wordToLearn._index, extraDurationForAnswer));
				}
			}
			else if (c == 77) // ������� ������
			{
				if (wordToLearn._fromWhatSource != FromWhatSource::FROM_LEANRING_QUEUE)
				{
					_learnWordsApp->add_forgotten(wordToLearn._index);
					_learnWordsApp->set_as_barely_known(w);
					pAdditionalCheck->put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
					_learnWordsApp->fill_dates_and_save(w, freezedTime, false, false);
					_learnWordsApp->erase_from_remembered_long(wordToLearn._index);
				}
			}
			else if (c == 80) // ������� ����
			{
				if (wordToLearn._fromWhatSource != FromWhatSource::FROM_LEANRING_QUEUE)
				{
					_learnWordsApp->add_forgotten(wordToLearn._index);
					_learnWordsApp->set_as_forgotten(w);
					pAdditionalCheck->put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
					_learnWordsApp->fill_dates_and_save(w, freezedTime, false, false);
					_learnWordsApp->erase_from_remembered_long(wordToLearn._index);
				}
			}
			else
				continue;
			break;
		}
	}
}


/*
			switch (wordToLearn._fromWhatSource)
			{
			case FromWhatSource::FROM_LEANRING_QUEUE:
				break;
			case FromWhatSource::FROM_REMEMBERED_LONG:
				break;
			case FromWhatSource::FROM_MANDATORY:
				break;
			case FromWhatSource::FROM_ADDITIONAL:
				break;
			}
if (c == 72)  // ������� �����
{
	switch (wordToLearn._fromWhatSource)
	{
	case FromWhatSource::FROM_LEANRING_QUEUE:
		if (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED)
		{
			if (are_all_words_learned(learnCycleQueue))
				return;
		}
		put_to_queue(learnCycleQueue, wordToLearn, false);
		break;
	case FromWhatSource::FROM_DISTRACTED:
		pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
		_learnWordsApp->save();
		break;
	}
	break;
}
else
if (c == 80) // ������� ����
{
	switch (wordToLearn._fromWhatSource)
	{
	case FromWhatSource::FROM_LEANRING_QUEUE:
		wordToLearn._localRightAnswersNum = 0;
		put_to_queue(learnCycleQueue, wordToLearn, false);
		break;
	case FromWhatSource::FROM_DISTRACTED:
		pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
		_learnWordsApp->add_forgotten(wordToLearn._index);
		_learnWordsApp->set_as_forgotten(w);
		_learnWordsApp->fill_dates_and_save(w, freezedTime, false, false);
		break;
	}
	break;
}
*/