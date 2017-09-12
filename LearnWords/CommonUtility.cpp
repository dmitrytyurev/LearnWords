#include "stdafx.h"
#define NOMINMAX
#include "windows.h"
#include <ctime>
#include <conio.h>

#include "CommonUtility.h"


//===============================================================================================
// 
//===============================================================================================

void _cdecl Log::operator()(char *text, ...)
{
	static char tmpStr[1024];
	va_list args;
	va_start(args, text);
	vsprintf_s(tmpStr, sizeof(tmpStr), text, args);
	va_end(args);

	FILE* f = nullptr;
	fopen_s(&f, _fileName.c_str(), "at");
	if (f == nullptr)
		exit_msg("Can't  create file %s\n", _fileName);
	fprintf(f, "%s", tmpStr);
	fclose(f);
}


//===============================================================================================
// 
//===============================================================================================

const char* get_time_in_text(time_t curTime)
{
	static char buf[128];
	struct tm timeinfo;
	localtime_s(&timeinfo, &curTime);
	asctime_s(buf, sizeof(buf), &timeinfo);
	return buf;
}


//===============================================================================================
// 
//===============================================================================================

void _cdecl exit_msg(char *text, ...)
{
	static char tmpStr[1024];
	va_list args;
	va_start(args, text);
	vsprintf_s(tmpStr, sizeof(tmpStr), text, args);
	va_end(args);

	printf("%s", tmpStr);
	exit(1);
}

//===============================================================================================
// 
//===============================================================================================

char getch_filtered()  // »гнорирует код -32 (встречаетс€, например, у стрелок)
{
	char c = 0;
	do
		c = _getch();
	while (c == -32);
	return c;
}


//===============================================================================================
// 
//===============================================================================================

void clear_console_screen(char fill)
{
	COORD tl = { 0,0 };
	CONSOLE_SCREEN_BUFFER_INFO s;
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(console, &s);
	DWORD written, cells = s.dwSize.X * s.dwSize.Y;
	FillConsoleOutputCharacter(console, fill, cells, tl, &written);
	FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
	SetConsoleCursorPosition(console, tl);
}

//===============================================================================================
// 
//===============================================================================================

int enter_number_from_console()
{
	char buffer[256] = { 0 };
	int index = 0;
	int printedSymbolsLastTime = 0;

	while (true)
	{
		char c = getch_filtered();
		if (c == 13) // Enter
			break;
		if (c >= '0' && c <= '9')
		{
			buffer[index++] = c;
			buffer[index] = 0;
		}

		if (c == 8)  // Backspace
		{
			if (index > 0)
				buffer[--index] = 0;
		}

		for (int i = 0; i < printedSymbolsLastTime; ++i)
			putchar(8);
		for (int i = 0; i < printedSymbolsLastTime; ++i)
			putchar(' ');
		for (int i = 0; i < printedSymbolsLastTime; ++i)
			putchar(8);
		printf("%s", buffer);
		printedSymbolsLastTime = strlen(buffer);
	}

	int number = 0;
	sscanf_s(buffer, "%d", &number);
	return number;
}
