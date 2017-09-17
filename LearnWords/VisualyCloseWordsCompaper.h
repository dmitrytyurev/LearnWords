#pragma once

struct VisualyCloseWordsCompaper
{
	VisualyCloseWordsCompaper();
	float calc_how_close(const std::string& str1, const std::string& str2);

	bool is_next_plus_one(const std::vector<int>& arr1, const std::vector<int>& arr2);
	bool is_next_greater(const std::vector<int>& arr1, const std::vector<int>& arr2);
	float calc_how_close_inner(const std::string& str1, const std::string& str2);

	float coeffs[9];
};