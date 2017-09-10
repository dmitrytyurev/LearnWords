#include "stdafx.h"

#include "CommonUtility.h"
#include "LearnWordsApp.h"

//===============================================================================================
// 
//===============================================================================================

const char* logFileName = "log.log";
Log logger(logFileName);

//===============================================================================================
// 
//===============================================================================================

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");
	std::srand(unsigned(std::time(nullptr)));
	srand((int)time(nullptr));

	LearnWordsApp app;
	app.process(argc, argv);

    return 0; 
}


