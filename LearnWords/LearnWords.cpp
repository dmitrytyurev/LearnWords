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

void test();

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Russian");

//test();
//return 0;

	

	LearnWordsApp app;
	app.process(argc, argv);

    return 0; 
}


