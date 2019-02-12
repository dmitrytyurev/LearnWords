#pragma once

struct LearnWordsApp;
struct WordsData;
struct AdditionalCheck;


// ��������� ����������� ����, ������������ ������������ ������� �����. �� �������� ��� ���������� �����, ������� ���� ������� ��������� :
// ��� ����� ������� ���� ��������� ��� ����� ����� ����, ����� ��� �����, �� ������� �� ������� �������� ���������, �� ����� ������

enum class FromWhatSource
{
	NOT_INITIALIZED,
	FROM_LEANRING_QUEUE,     // ����� �� ������� ������������ ����
	FROM_REMEMBERED_LONG,    // ��� ����� �� ���, ��� ��������� � ���� ������ ���������, �������� ������, �� ����� �����
	FROM_MANDATORY,          // ��� ����� �� ������� ������������ �������� (������� ��������� ����������)
	FROM_ADDITIONAL          // ��� ����� �������� �� AdditionalCheck
};

struct DistractWord
{

	DistractWord(int wordIndex, FromWhatSource wordFromWhatSource) : index(wordIndex), fromWhatSource(wordFromWhatSource) {}

	int index = 0;                                                   // ������ ����� � _wordsOnDisk
	FromWhatSource fromWhatSource = FromWhatSource::NOT_INITIALIZED; // �������� �����
};

class DistractWordsSupplier  
{
public:
	DistractWordsSupplier(LearnWordsApp* learnWordsApp, time_t freezedTime);
	DistractWord get_word(time_t freezedTime, AdditionalCheck* pAdditionalCheck);
	bool is_first_cycle();

private:
	LearnWordsApp* _learnWordsApp = nullptr;
	std::vector<DistractWord> distractWords; // ����� ��� ����������. ���� �� �� ������, �� ����� ����������� �������� �� ������������ ������ �������
	int index = 0;                     // ������ ����������� ����� � distractWords
	int returnWordsFromAdditional = 0; // ������� ���� ����� ������� �� AdditionalCheck. ���� ����� 0, �� ����� ����� �� distractWords.
	bool isFirstCycle = true;          // ������ �� ��� ���� ������ �� ������� distractWords
};


// ����� ��� ���������� ����� ���� � ����������� ������� 

struct LearnNew
{
	struct WordToLearn
	{
		WordToLearn() {}
		explicit WordToLearn(int index, FromWhatSource fromWhatSource) : _index(index), _fromWhatSource(fromWhatSource) {}

		int  _index = 0;                 // ������ ���������� ����� � WordsOnDisk::_words
		int  _localRightAnswersNum = 0;  // ���������� ����������� ���������� �������
		FromWhatSource _fromWhatSource = FromWhatSource::NOT_INITIALIZED;
	};

	LearnNew(LearnWordsApp* learnWordsApp, WordsData* pWordsData) : _learnWordsApp(learnWordsApp), _pWordsData(pWordsData) {}
	void learn_new(time_t freezedTime, AdditionalCheck* pAdditionalCheck);
	void learn_forgotten(time_t freezedTime, AdditionalCheck* pAdditionalCheck);
	
	void print_masked_translation(const char* _str, int symbolsToShowNum);
	void put_to_queue(std::vector<WordToLearn>& queue, const WordToLearn& wordToPut, bool needRandomInsert);
	bool are_all_words_learned(std::vector<WordToLearn>& queue);

	WordsData*     _pWordsData;
	LearnWordsApp* _learnWordsApp;
};

