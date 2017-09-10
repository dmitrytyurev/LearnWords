#include "stdafx.h"
#include <vector>
#include <algorithm>

#include "LearnWordsApp.h"
#include "CommonUtility.h"
#include "CloseTranslationWordsManager.h"
#include "RepeatOfRecent.h"


//===============================================================================================
// 
//===============================================================================================

void RepeatOfRecent::repeat_of_recent(time_t freezedTime)
{
	_learnWordsApp->clear_forgotten();
	clear_console_screen();

	std::vector<int> wordsToRepeat;

	// Выбрать слова для проверки (недавно изученные и забытые)

	for (int i = 0; i < (int)_pWordsData->_words.size(); ++i)
	{
		WordsData::WordInfo& w = _pWordsData->_words[i];
		if (_learnWordsApp->isWordJustLearnedOrForgotten(w, freezedTime))
			wordsToRepeat.push_back(i);
	}

	if (wordsToRepeat.size() == 0)
		return;

	// Главный цикл проверки слов

	std::random_shuffle(wordsToRepeat.begin(), wordsToRepeat.end());
	for (int i = 0; i < (int)wordsToRepeat.size(); ++i)
	{
		WordsData::WordInfo& w = _pWordsData->_words[wordsToRepeat[i]];

		clear_console_screen();
		printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
		char c = 0;
		do
		{
			c = getch_filtered();
			if (c == 27)
				return;
		} while (c != ' ');

		while (true)
		{
			clear_console_screen();
			printf("\n\n===============================\n %s\n===============================\n", w.word.c_str());
			_learnWordsApp->print_buttons_hints(w.translation, false);
			printf("\n  Осталось: %d, rightAnswersNum=%d\n", (int)wordsToRepeat.size() - i - 1, w.rightAnswersNum);
			CloseTranslationWordsManager ctwm(_learnWordsApp, _pWordsData, wordsToRepeat[i]);
			ctwm.print_close_words_by_translation();

			c = getch_filtered();
			ctwm.process_user_input(c);
			if (c == 27)
				return;
			if (c == 72)  // Стрелка вверх
			{
				w.rightAnswersNum = 1;
				_learnWordsApp->fill_dates_and_save(w, freezedTime, LearnWordsApp::RandScopePart::ALL);
				break;
			}
			else
				if (c == 80) // Стрелка вниз
				{
					_learnWordsApp->add_forgotten(wordsToRepeat[i]);
					_learnWordsApp->set_word_as_just_learned(w);
					_learnWordsApp->fill_dates_and_save(w, freezedTime, LearnWordsApp::RandScopePart::ALL);
					break;
				}
		}
	}
}
