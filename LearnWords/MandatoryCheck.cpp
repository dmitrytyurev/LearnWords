#include "stdafx.h"
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <chrono>

#include "LearnWordsApp.h"
#include "CommonUtility.h"
#include "AdditionalCheck.h"
#include "CloseTranslationWordsManager.h"
#include "MandatoryCheck.h"

const int RIGHT_ANSWERS_FALLBACK = 10;            // Номер шага, на который откатывается слово при check_by_time, если помним неуверенно
extern Log logger;

//===============================================================================================
// 
//===============================================================================================


void MandatoryCheck::mandatory_check(time_t freezedTime, AdditionalCheck* pAdditionalCheck, const std::string& fullFileName)
{
	_learnWordsApp->clear_forgotten();
	clear_console_screen();

	struct WordToCheck
	{
		WordToCheck() : _index(0), _sortCoeff(0) {}
		WordToCheck(int index) : _index(index), _sortCoeff(0) {}

		int   _index;       // Индекс повторяемого слова в WordsOnDisk::_words
		float _sortCoeff;   // Сортировочный коэффициент, если пора повторять больше слов, чем мы хотим сейчас повторять
	};
	std::vector<WordToCheck> wordsToRepeat;

	// Выбрать слова для проверки, для которых подошло время проверки

	for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)
	{
		WordsData::WordInfo& w = _pWordsData->_words[i];
		if (w.dateOfRepeat != 0 && w.dateOfRepeat < freezedTime)
			wordsToRepeat.push_back(WordToCheck(i));
	}

	// Если этих слов больше, чем нужно, то урезать число слов до необходимого

	const int MAX_WORDS_TO_CHECK = 77;
	if (wordsToRepeat.size() > MAX_WORDS_TO_CHECK)
	{
		for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
		{
			WordsData::WordInfo& w = _pWordsData->_words[wordsToRepeat[i]._index];
			int plannedRepeatInterval = (w.dateOfRepeat - w.cantRandomTestedAfter) * 2;;  // Запланированный интервал повтора
			int lateRepeatInterval = int(freezedTime) - w.dateOfRepeat;  // На сколько просрочен повтор от запланированного времени
			wordsToRepeat[i]._sortCoeff = lateRepeatInterval / (plannedRepeatInterval * 0.2f + 1);  // Чем больше коэффициент, тем раньше надо повторять слова
		}
		std::sort(wordsToRepeat.begin(), wordsToRepeat.end(), [](const WordToCheck& l, const WordToCheck& r)  { return l._sortCoeff > r._sortCoeff; });
		wordsToRepeat.resize(MAX_WORDS_TO_CHECK);
	}

	// Перемешать выбранные слова

	std::random_shuffle(wordsToRepeat.begin(), wordsToRepeat.end());

	// Главный цикл проверки слов

	for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
	{
		WordsData::WordInfo& w = _pWordsData->_words[wordsToRepeat[i]._index];

		clear_console_screen();
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
		bool isQuickAnswer = _learnWordsApp->is_quick_answer(std::chrono::duration<double, std::milli>(t_end - t_start).count());

		while (true)
		{
			clear_console_screen();
			printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
			_learnWordsApp->print_buttons_hints(w.translation, true);
			printf("\n  Осталось: %d, Быстрый ответ = %d, rightAnswersNum=%d\n", (int)wordsToRepeat.size() - i - 1, int(isQuickAnswer), w.rightAnswersNum);

			CloseTranslationWordsManager ctwm(_learnWordsApp, _pWordsData, wordsToRepeat[i]._index);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;                              //FIXME!!! внутри условий ниже много повторов!
			int keepPrevRightAnswersNum = w.rightAnswersNum;
			if (c == 72)  // Стрелка вверх
			{
					pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
					LearnWordsApp::RandScopePart randScopePart = LearnWordsApp::RandScopePart::LOWER_PART;
					if (isQuickAnswer)
					{
						randScopePart = LearnWordsApp::RandScopePart::HI_PART;
						w.isNeedSkipOneRandomLoop = true;
					}

int keep = w.rightAnswersNum;
				_learnWordsApp->fill_rightAnswersNum(w, isQuickAnswer);
if (w.rightAnswersNum - keep > 1)
{
	clear_console_screen();
	printf("Moved to %d!\n", w.rightAnswersNum - keep);
	Sleep(1200);
}
					_learnWordsApp->fill_dates_and_save(w, freezedTime, randScopePart);
				}
				else
				if (c == 80) // Стрелка вниз
				{
					keepPrevRightAnswersNum = w.rightAnswersNum;
					_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
						_learnWordsApp->set_word_as_just_learned(w);
						_learnWordsApp->fill_dates_and_save(w, freezedTime, LearnWordsApp::RandScopePart::ALL);
					}
					else
					if (c == 77) // Стрелка вправо (помним слово не очень уверенно)
					{
						_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
							w.rightAnswersNum = std::min(w.rightAnswersNum, RIGHT_ANSWERS_FALLBACK);
							pAdditionalCheck->put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
							_learnWordsApp->fill_dates_and_save(w, freezedTime, LearnWordsApp::RandScopePart::ALL);
						}
						else
						continue;
			logger("Check by time, word = %s, key=%d, PrevRightAnswersNum=%d, time = %s", w.word.c_str(), c, keepPrevRightAnswersNum, get_time_in_text(time(nullptr)));
			break;
		}
	}
}
