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

const int RIGHT_ANSWERS_FALLBACK = 6;            // Ќомер шага, на который откатываетс€ слово при check_by_time, если помним неуверенно
extern Log logger;

//===============================================================================================
// 
//===============================================================================================

float MandatoryCheck::calc_additional_word_probability(int checkByTimeWordsNumber)
{
	const float MIN_PROB = 0.1f;
	const float MAX_PROB = 0.9f;
	const float MIN_WORDS = 25;
	const float MAX_WORDS = 50;

	float prob = interp(MAX_WORDS, MIN_PROB, MIN_WORDS, MAX_PROB, (float)checkByTimeWordsNumber);   // –ассчитаем веро€тность вставки доп. слов в зависимости от числа слов дл€ об€зательной проверки по времени
	prob = std::max(MIN_PROB, prob);
	prob = std::min(MAX_PROB, prob);
	return prob;
}


//===============================================================================================
// 
//===============================================================================================


void MandatoryCheck::mandatory_check(time_t freezedTime, AdditionalCheck* pAdditionalCheck, const std::string& fullFileName)
{
	enum class FromWhatSource
	{
		DEFAULT,
		CHECK_BY_TIME,
		RANDOM_REPEAT,
	};

	struct WordToCheck
	{
		WordToCheck() : _index(0), _fromWhatSource(FromWhatSource::DEFAULT) {}
		WordToCheck(int index, FromWhatSource fromWhatSource) : _index(index), _fromWhatSource(fromWhatSource) {}

		int  _index;                     // »ндекс повтор€емого слова в WordsOnDisk::_words  (если _fromWhatSource == RANDOM_REPEAT, то здесь всегда 0)
		FromWhatSource _fromWhatSource;  // “ип слова
	};

	_learnWordsApp->clear_forgotten();
	clear_console_screen();

	std::vector<WordToCheck> wordsToRepeat;

	// ¬ыбрать слова дл€ проверки, дл€ которых подошло врем€ проверки

	for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)
	{
		WordsData::WordInfo& w = _pWordsData->_words[i];
		if (w.dateOfRepeat != 0 && w.dateOfRepeat < freezedTime)
		{
			WordToCheck wordToCheck(i, FromWhatSource::CHECK_BY_TIME);
			wordsToRepeat.push_back(wordToCheck);
		}
	}
	std::random_shuffle(wordsToRepeat.begin(), wordsToRepeat.end());

	// ћежду слов, которые нужно проверить по времени, повставл€ем слова дл€ случайной проверки
	float prob = calc_additional_word_probability((int)wordsToRepeat.size());

	for (int i = 0; i < (int)wordsToRepeat.size() - 1; ++i)
	{
		if (rand_float(0, 1) < prob)
		{
			WordToCheck wordToCheck(0, FromWhatSource::RANDOM_REPEAT);   // »ндекс слова не записываем, только его тип! »ндекс получим по ходу проверки
			wordsToRepeat.insert(wordsToRepeat.begin() + i + 1, wordToCheck);
			++i;
		}
	}

	// √лавный цикл проверки слов

	for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
	{
		if (wordsToRepeat[i]._fromWhatSource == FromWhatSource::RANDOM_REPEAT)
		{
			wordsToRepeat[i]._index = pAdditionalCheck->get_word_to_repeat(freezedTime);
			if (wordsToRepeat[i]._index == -1)
				continue;
		}

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
			printf("\n  ќсталось: %d, Ѕыстрый ответ = %d, rightAnswersNum=%d\n", (int)wordsToRepeat.size() - i - 1, int(isQuickAnswer), w.rightAnswersNum);

			CloseTranslationWordsManager ctwm(_learnWordsApp, _pWordsData, wordsToRepeat[i]._index);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;                              //FIXME!!! внутри условий ниже много повторов!
			int keepPrevRightAnswersNum = 0;
			if (c == 72)  // —трелка вверх
			{
				if (wordsToRepeat[i]._fromWhatSource == FromWhatSource::CHECK_BY_TIME)
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
	Sleep(3000);
}
					_learnWordsApp->fill_dates_and_save(w, freezedTime, randScopePart);
				}
				else
				{
					pAdditionalCheck->put_word_to_end_of_random_repeat_queue_common(w);
					if (isQuickAnswer)
						w.isNeedSkipOneRandomLoop = true;
					_learnWordsApp->save();
				}
			}
			else
				if (c == 80) // —трелка вниз
				{
					if (wordsToRepeat[i]._fromWhatSource == FromWhatSource::CHECK_BY_TIME)
					{
						_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
						_learnWordsApp->set_word_as_just_learned(w);
						_learnWordsApp->fill_dates_and_save(w, freezedTime, LearnWordsApp::RandScopePart::ALL);
					}
					else
					{
						_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
						_learnWordsApp->set_word_as_just_learned(w);
						_learnWordsApp->fill_dates_and_save(w, freezedTime, LearnWordsApp::RandScopePart::ALL);
					}
				}
				else
					if (c == 77) // —трелка вправо (помним слово не очень уверенно)
					{
						if (wordsToRepeat[i]._fromWhatSource == FromWhatSource::CHECK_BY_TIME)
						{
							_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
							w.rightAnswersNum = std::min(w.rightAnswersNum, RIGHT_ANSWERS_FALLBACK);
							pAdditionalCheck->put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
							_learnWordsApp->fill_dates_and_save(w, freezedTime, LearnWordsApp::RandScopePart::ALL);
						}
						else
						{
							_learnWordsApp->add_forgotten(wordsToRepeat[i]._index);
							pAdditionalCheck->put_word_to_end_of_random_repeat_queue_fast(w, freezedTime);
							_learnWordsApp->save();
						}
					}
					else
						continue;
			logger("Check by time, word = %s, ===== %s, src=%d, key=%d, time = %s", w.word.c_str(), fullFileName.c_str(), wordsToRepeat[i]._fromWhatSource, c, get_time_in_text(time(nullptr)));
			break;
		}
	}
}
