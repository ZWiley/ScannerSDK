// SDKTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
//#include "ScannerSDK.h"
#include "YiruiScanner.h"
#include <iostream>

//#pragma comment(lib,"../x64/Release/ScannerSDK.lib") 
#pragma comment(lib,"../x64/Release/YiruiScanner.lib") 

int main()
{
	const char* scannerDeviceName = "COM3";
	if (openScanner(scannerDeviceName) != 0) {
		std::cout << "open device failed!" << std::endl;
		exit(-1);
	}
	std::cout << "open device success!" << std::endl;

	const char* res = readScanner();
	std::cout << "res:" << res;

	closeScanner();
	system("pause");
    return 0;
}