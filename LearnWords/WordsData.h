#pragma once

#include "stdafx.h"
#include <vector>
#include <string>

struct WordsData
{
	struct WordInfo
	{
		WordInfo() { clear_all(); }
		void clear_all()
		{
			rightAnswersNum = 0;
			dateOfRepeat = 0;
			cantRandomTestedAfter = 0;
			randomTestIncID = 0;
			cantRandomTestedBefore = 0;
			isInFastRandomQueue = false;
			isNeedSkipOneRandomLoop = false;
		}

		std::string word;
		std::string translation;
		int rightAnswersNum;       // ��-������� 0 - ���� ����� ��� �� �����
								   // ������� ��� ����� ���� ������� ���������. ����� ���������� �������� rightAnswersNum = 1. 
		int dateOfRepeat;          // ����, ����� ����� ��������� ����� (����� � �������� � 1970 ����). ��-������� = 0. ����� �����, ���� rightAnswersNum >= 1

		int cantRandomTestedAfter; // ���� !=0, �� ����� ����� ������� ������ ������������ ����� � ��������� �����, ���� ����� ��� �������� �������� �����. ����� ����� �� ������ ����.
		int randomTestIncID;       // � ������ ��� �������� ����������� ����� �������� �� 1 ������, ��� � ���� ���������. 
		bool isInFastRandomQueue;  // ��������� �� ����� � ������� ������� ���������� �������
		bool isNeedSkipOneRandomLoop; // ����� �� ���������� ���� ���� ���������� ������� �����
		int  cantRandomTestedBefore;  // ���� !=0, �� �� ����� ������� ������ ������������ ����� � ��������� �����, ��������� �� ������� ��� ��������� � ����� ������� ������� ����� ��� ����� ��������
	};

	struct CompareExcludePair
	{
		std::string word1;
		std::string word2;
	};

	struct ListeningTextToKeep
	{
		std::string path;  // ���� � ���-��������, �� �������� �����
		int volumeN;       // ����� ����� (������������� �������� ����� � ���-��������
		int phraseN;       // ����� ����� � �����
	};

	std::vector<WordInfo> _words;
	std::vector<CompareExcludePair> _compareExcludePairs;
	std::vector<ListeningTextToKeep> _listeningTextsToKeep;
};