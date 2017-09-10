#pragma once
#include "string"

const int SECONDS_IN_DAY = 3600 * 24;

const char* get_time_in_text(time_t curTime);
void _cdecl exit_msg(char *text, ...);
char getch_filtered();  // ���������� ��� -32 (�����������, ��������, � �������)
void clear_console_screen(char fill = ' ');
int enter_number_from_console();

//===============================================================================================
// 
//===============================================================================================

struct Log
{
	explicit Log(const char* fileName): _fileName(fileName) {}
	void _cdecl operator()(char *text, ...);

	std::string _fileName;
};


//===============================================================================================
// 
//===============================================================================================

inline bool is_digit(char c)
{
	return  c >= '0'  &&  c <= '9';
}

//===============================================================================================
// 
//===============================================================================================

inline float interp(float x1, float y1, float x2, float y2, float x)
{
	float k = (y2 - y1) / (x2 - x1);
	float b = y1 - x1*k;
	return x*k + b;
}

//===============================================================================================
// 
//===============================================================================================

inline float interp_clip(float x1, float y1Min, float x2, float y2Max, float x)
{
	float y = interp(x1, y1Min, x2, y2Max, x);

	if (y < y1Min)
		return y1Min;

	if (y > y2Max)
		return y2Max;

	return y;
}

//===============================================================================================
// 
//===============================================================================================

inline int rand_int(int min, int max)
{
	return   rand() * (max - min + 1) / (RAND_MAX + 1) + min;
}

//===============================================================================================
// 
//===============================================================================================

inline float rand_float(float min, float max)
{
	return   rand() * (max - min) / RAND_MAX + min;
}


//===============================================================================================
// 
//===============================================================================================

inline bool is_russian_symbol(unsigned int c)
{
	if ((c >= 224 && c <= 255) ||
		(c >= 192 && c <= 223) ||
		c == 184 ||
		c == 168)
		return true;
	return false;
}

//===============================================================================================
// 
//===============================================================================================

inline bool is_symbol(unsigned int c)
{
	if (is_russian_symbol(c))
		return true;

	return (c >= 'a'  &&  c <= 'z') || (c >= 'A'  &&  c <= 'Z');
}
