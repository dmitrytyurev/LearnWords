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

extern Log logger;

//===============================================================================================
// 
//===============================================================================================

const int LAST_HOURS_REPEAT_NUM = 7;  // Если на повтор слов мало, то добавим слова из повторённых за столько последних часов

void MandatoryCheck::mandatory_check(time_t freezedTime, AdditionalCheck* pAdditionalCheck, const std::string& fullFileName)
{
	_learnWordsApp->clear_forgotten();
	clear_console_screen();

	struct WordToCheck
	{
		WordToCheck() : _index(0), _sortCoeff(0), _ifRecentlyRepeated(false) {}
		WordToCheck(int index) : _index(index), _sortCoeff(0), _ifRecentlyRepeated(false) {}
		WordToCheck(int index, bool ifRecentlyRepeated) : _index(index), _sortCoeff(0), _ifRecentlyRepeated(ifRecentlyRepeated) {}

		int   _index;       // Индекс повторяемого слова в WordsOnDisk::_words
		float _sortCoeff;   // Сортировочный коэффициент, если пора повторять больше слов, чем мы хотим сейчас повторять
		bool  _ifRecentlyRepeated;  // true, если слово повторялось за последние LAST_HOURS_REPEAT_NUM часов
	};
	std::vector<WordToCheck> wordsToRepeat;

	// Выбрать слова для проверки, для которых подошло время проверки

	auto ifTimeToRepeat = [](WordsData::WordInfo& w, time_t freezedTime) { return w.dateOfRepeat != 0 && w.dateOfRepeat < freezedTime; };

	for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)
	{
		WordsData::WordInfo& w = _pWordsData->_words[i];
		if (ifTimeToRepeat(w, freezedTime))
			wordsToRepeat.emplace_back(WordToCheck(i));
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
	else
		if (wordsToRepeat.size() < MAX_WORDS_TO_CHECK)  // Если слов наоборот меньше, то добавить
		{
			auto ifRepeatedRecently = [](WordsData::WordInfo& w, time_t freezedTime) { return freezedTime - w.calcPrevRepeatTime() < 3600 * LAST_HOURS_REPEAT_NUM; };

			for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)   // Сначала добавляем слова, которые повторяли за последние LAST_HOURS_REPEAT_NUM часов
			{
				WordsData::WordInfo& w = _pWordsData->_words[i];

				if (!ifTimeToRepeat(w, freezedTime) && ifRepeatedRecently(w, freezedTime))
				{
logger("Add from recently: %s, time from repeat: %d\n", w.word.c_str(), freezedTime - w.calcPrevRepeatTime());
					wordsToRepeat.emplace_back(WordToCheck(i, true));
					if (wordsToRepeat.size() == MAX_WORDS_TO_CHECK)
						break;
				}
			}

			if (wordsToRepeat.size() < MAX_WORDS_TO_CHECK)  // Если слов по-прежнему не хватает, то добавляем слова, время повтора которых придёт через NN часов
			{
				const int PRELIMINARY_CHECK_HOURS = 24;  // Слова, которые надо будет проверять через столько часов добавим в проверку сейчас, если слов не хватает

				for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)
				{
					WordsData::WordInfo& w = _pWordsData->_words[i];

					if (!ifTimeToRepeat(w, freezedTime) && !ifRepeatedRecently(w, freezedTime))
					{
						if (w.dateOfRepeat != 0 && w.dateOfRepeat < freezedTime + 3600 * PRELIMINARY_CHECK_HOURS)
						{
logger("Add from future: %s, time to repeat: %d\n", w.word.c_str(), w.dateOfRepeat - freezedTime);
							wordsToRepeat.emplace_back(WordToCheck(i));
							if (wordsToRepeat.size() == MAX_WORDS_TO_CHECK)
								break;
						}
					}
				}
			}
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
		bool isQuickAnswer = _learnWordsApp->is_quick_answer(std::chrono::duration<double, std::milli>(t_end - t_start).count(), w.translation.c_str());

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
				if (isQuickAnswer)
					w.isNeedSkipOneRandomLoop = true;
				_learnWordsApp->fill_dates_and_save(w, freezedTime, wordsToRepeat[i]._ifRecentlyRepeated == false, isQuickAnswer);
			}
			else
				if (c == 80) // Стрелка вниз
				{
					_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
					_learnWordsApp->set_as_forgotten(w);
					pAdditionalCheck->put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
					_learnWordsApp->fill_dates_and_save(w, freezedTime, false, false);
				}
				else
					if (c == 77) // Стрелка вправо (помним слово не очень уверенно)
					{
						_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
						_learnWordsApp->set_as_barely_known(w);
						pAdditionalCheck->put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
						_learnWordsApp->fill_dates_and_save(w, freezedTime, false, false);
					}
					else
						continue;
			logger("Check by time, word = %s, key=%d, PrevRightAnswersNum=%d, time = %s", w.word.c_str(), c, keepPrevRightAnswersNum, get_time_in_text(time(nullptr)));
			break;
		}
	}
}
