#include "stdafx.h"
#include <vector>

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
		WordToLearn word;
		word._index = index;
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
		enum class FromWhatSource
		{
			DEFAULT,
			FROM_LEANRING_QUEUE,
			FROM_RANDOM_REPEAT_LIST,
		} fromWhatSource = FromWhatSource::DEFAULT;

		int wordToRepeatIndex = pAdditionalCheck->get_word_to_repeat(freezedTime);

		if (rand_float(0, 1) > treshold || wordToRepeatIndex == -1)
		{
			fromWhatSource = FromWhatSource::FROM_LEANRING_QUEUE;
			wordToLearn = learnCycleQueue[0];
			learnCycleQueue.erase(learnCycleQueue.begin());
		}
		else
		{
			fromWhatSource = FromWhatSource::FROM_RANDOM_REPEAT_LIST;
			wordToLearn = WordToLearn(wordToRepeatIndex);
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
				switch (fromWhatSource)
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
				case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
					pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
					_learnWordsApp->save();
					break;
				}
				break;
			}
			else
				if (c == 80) // ������� ����
				{
					switch (fromWhatSource)
					{
					case FromWhatSource::FROM_LEANRING_QUEUE:
						wordToLearn._localRightAnswersNum = 0;
						put_to_queue(learnCycleQueue, wordToLearn, true);
						w.clear_all();
						_learnWordsApp->save();
						break;
					case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
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

	std::vector<WordToLearn> learnCycleQueue;  // ����������� ������� ���� � �������� �������� (��������� � �����, ���� �� ������)

											   // ������� �����, ������� ����� ������� � �������
	for (const auto& index : wordsToLearnIndices)
	{
		WordToLearn word;
		word._index = index;
		learnCycleQueue.push_back(word);
	}

	int showFromRandomNum = rand_int(0, 3);
	while (true)
	{
		clear_console_screen();

		// ������� �����, ������� ����� ����������
		WordToLearn wordToLearn;
		enum class FromWhatSource
		{
			DEFAULT,
			FROM_LEANRING_QUEUE,
			FROM_RANDOM_REPEAT_LIST,
		} fromWhatSource = FromWhatSource::DEFAULT;

		int wordToRepeatIndex = pAdditionalCheck->get_word_to_repeat(freezedTime);

		bool wantShowLearnWord = (--showFromRandomNum == -1);
		if (wantShowLearnWord || wordToRepeatIndex == -1)
		{
			showFromRandomNum = rand_int(1, 3);
			fromWhatSource = FromWhatSource::FROM_LEANRING_QUEUE;
			wordToLearn = learnCycleQueue[0];
			learnCycleQueue.erase(learnCycleQueue.begin());
		}
		else
		{
			fromWhatSource = FromWhatSource::FROM_RANDOM_REPEAT_LIST;
			wordToLearn = WordToLearn(wordToRepeatIndex);
		}

		// ������� � ���������� �������� � �������
		int sumRightUnswers = 0;
		if (fromWhatSource == FromWhatSource::FROM_LEANRING_QUEUE)
			sumRightUnswers += wordToLearn._localRightAnswersNum;
		for (const auto& word : learnCycleQueue)
			sumRightUnswers += word._localRightAnswersNum;
		int progress = sumRightUnswers * 100 / (wordsToLearnIndices.size() * TIMES_TO_GUESS_TO_LEARNED);
		printf("\nProgress = %d\n", progress);

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
				switch (fromWhatSource)
				{
				case FromWhatSource::FROM_LEANRING_QUEUE:
					if (++(wordToLearn._localRightAnswersNum) == TIMES_TO_GUESS_TO_LEARNED)
					{
						if (are_all_words_learned(learnCycleQueue))
							return;
					}
					put_to_queue(learnCycleQueue, wordToLearn, false);
					break;
				case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
					pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
					_learnWordsApp->save();
					break;
				}
				break;
			}
			else
				if (c == 80) // ������� ����
				{
					switch (fromWhatSource)
					{
					case FromWhatSource::FROM_LEANRING_QUEUE:
						wordToLearn._localRightAnswersNum = 0;
						put_to_queue(learnCycleQueue, wordToLearn, false);
						break;
					case FromWhatSource::FROM_RANDOM_REPEAT_LIST:
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
