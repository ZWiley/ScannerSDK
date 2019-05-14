// ScannerSDK.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "ScannerSDK.h"
#include "serialport.h"

#include <iostream>
#include <string>

HANDLE hPort = nullptr;

int stringToInt(const char* p) {
	int value = 0;
	while (*p != '\0')
	{
		if ((*p >= '0') && (*p <= '9'))
		{
			value = value * 10 + *p - '0';
		}
		p++;
	}
	return value;
}

DWORD readFromSerialPort(HANDLE hSerial, char * buffer, int buffersize)
{
	DWORD dwBytesRead = 0;
	if (!ReadFile(hSerial, buffer, buffersize, &dwBytesRead, NULL)) {
		int errorCode = GetLastError();
		std::cout << "ReadFile error: %d" << errorCode << std::endl;
	}
	return dwBytesRead;
}

void closeSerialPort(HANDLE hSerial)
{
	CloseHandle(hSerial);
}

int openScanner(const char* portName)
{
	int portNum = stringToInt(portName);
	if (portNum > 9)
	{
		hPort = ::CreateFileA((std::string("\\\\.\\") + std::string(portName)).c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
		std::cout << std::string("\\\\.\\") + std::string(portName) << std::endl;
	} 
	else
	{
		hPort = ::CreateFileA((std::string(portName)).c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
		std::cout << std::string(portName) << std::endl;
	}
		
	if (hPort == INVALID_HANDLE_VALUE) {
		std::cout << "invalid port!" << std::endl;
		if (GetLastError() != NO_ERROR) {
			int errorCode0 = GetLastError();
			std::cout << "invalid port error: %d" << errorCode0 << std::endl;
			return -1;
		}
	}
	else {
		//CloseHandle(hPort);
		DCB dcb = { 0 };
		memset(&dcb, 0, sizeof(dcb));
		dcb.DCBlength = sizeof(dcb);
		if (!GetCommState(hPort, &dcb)) {
			int errorCode1 = GetLastError();
			std::cout << "GetCommState error: %d" << errorCode1 << std::endl;
		}

		dcb.BaudRate		= CBR_9600;
		dcb.Parity			= NOPARITY;
		dcb.ByteSize		= 8;
		dcb.StopBits		= ONESTOPBIT;
		dcb.fDtrControl		= DTR_CONTROL_DISABLE;
		dcb.fBinary			= TRUE;
		dcb.fParity			= FALSE;
		dcb.fErrorChar		= 0;
		dcb.fNull			= 0;
		dcb.fAbortOnError	= 0;
		dcb.wReserved		= 0;
		dcb.XonLim			= 2;
		dcb.XoffLim			= 4;
		dcb.XonChar			= 0x13;
		dcb.XoffChar		= 0x19;
		dcb.EvtChar			= 0;

		if (!SetCommState(hPort, &dcb)) {
			int errorCode2 = GetLastError();
			std::cout << "SetCommState error: %d" << errorCode2 << std::endl;
		}

		COMMTIMEOUTS timeouts				 = { 0 };
		timeouts.ReadIntervalTimeout		 = 50;
		timeouts.ReadTotalTimeoutConstant	 = 100;
		timeouts.ReadTotalTimeoutMultiplier  = 20;
		timeouts.WriteTotalTimeoutConstant	 = 100;
		timeouts.WriteTotalTimeoutMultiplier = 20;

		if (!SetCommTimeouts(hPort, &timeouts)) {
			int errorCode3 = GetLastError();
			std::cout << "SetCommTimeouts error: %d" << errorCode3 << std::endl;
		}
		return 0;
	}
}

const char* readScanner()
{
	// how to alloc this buffer?
	char readbuffer[2048];
	int bytesRead = readFromSerialPort(hPort, readbuffer, 2048);
	return readbuffer;
}

void closeScanner()
{
	closeSerialPort(hPort);
}
