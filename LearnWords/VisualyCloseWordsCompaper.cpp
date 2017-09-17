#include "stdafx.h"
#include <vector>

#include "VisualyCloseWordsCompaper.h"


VisualyCloseWordsCompaper::VisualyCloseWordsCompaper()
{
	*((int*)(&coeffs[0])) = 0x3f1ccccc;
	*((int*)(&coeffs[1])) = 0x3e828f5c;
	*((int*)(&coeffs[2])) = 0x3efae147;
	*((int*)(&coeffs[3])) = 0x3ebc28f5;
	*((int*)(&coeffs[4])) = 0x3e7ae147;
	*((int*)(&coeffs[5])) = 0x0;
	*((int*)(&coeffs[6])) = 0x3f1ccccc;
	*((int*)(&coeffs[7])) = 0x3ebc28f5;
	*((int*)(&coeffs[8])) = 0x0;
}


//===============================================================================================
// 
//===============================================================================================

bool VisualyCloseWordsCompaper::is_next_plus_one(const std::vector<int>& arr1, const std::vector<int>& arr2)
{
	for (int val1 : arr1)
	{
		for (int val2 : arr2)
		{
			if (val2 == val1 + 1)
				return true;
		}
	}
	return false;
}

//===============================================================================================
// 
//===============================================================================================

bool VisualyCloseWordsCompaper::is_next_greater(const std::vector<int>& arr1, const std::vector<int>& arr2)
{
	for (int val1 : arr1)
	{
		for (int val2 : arr2)
		{
			if (val2 > val1)
				return true;
		}
	}
	return false;
}

//===============================================================================================
// 
//===============================================================================================

float VisualyCloseWordsCompaper::calc_how_close_inner(const std::string& str1, const std::string& str2)
{
	int sizeN1 = str1.size();
	int sizeN2 = str2.size();

	// Заполнить charIstancies

	std::vector<std::vector<int>> charIstancies(sizeN1);  // Для каждого символа str1 индексы таких же символов в str2

	for (int i1 = 0; i1 < sizeN1; ++i1)
	{
		for (int i2 = 0; i2 < sizeN2; ++i2)
		{
			if (str1[i1] == str2[i2])
				charIstancies[i1].push_back(i2);
		}
	}

	// Подсчёт похожести

	float closeScore = 0;

	if (str1[0] == str2[0])
		closeScore += coeffs[0];

	int diff = abs(sizeN1 - sizeN2);

	if (diff == 0)
		closeScore += coeffs[6];
	else
		if (diff == 1)
			closeScore += coeffs[7];

	for (int i = 0; i < sizeN1; ++i)
	{
		const auto& val = charIstancies[i];
		if (val.empty())
			closeScore -= coeffs[1];
		else
		{
			if (i < sizeN1 - 1)
			{
				if (is_next_plus_one(val, charIstancies[i + 1]))
					closeScore += coeffs[2];
				else
				{
					if (is_next_greater(val, charIstancies[i + 1]))
						closeScore += coeffs[3];
					else
					{
						if (i < sizeN1 - 2 && is_next_greater(val, charIstancies[i + 2]))
							closeScore += coeffs[4];
					}
				}
			}
		}
	}

	return closeScore;
}

//===============================================================================================
// 
//===============================================================================================

float VisualyCloseWordsCompaper::calc_how_close(const std::string& str1, const std::string& str2)
{
	float val1 = calc_how_close_inner(str1, str2);
	float val2 = calc_how_close_inner(str2, str1);

	return (val1 + val2) * 0.5f;
}
