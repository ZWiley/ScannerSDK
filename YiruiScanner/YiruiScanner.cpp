#include "stdafx.h"
#include "YiruiScanner.h"
#include "WinCom.h"

#include <iostream>

WinCom *m_pCom = new WinCom();

int	openScanner(const char* portName) {
	m_pCom = new WinCom();
	if (m_pCom->Open(portName, CBR_9600))
	{
		return 0;
	}
	else
	{
		return -1;
	}
	
}

const char* readScanner() {
	const char* res = m_pCom->Read();
	return res;
}

void closeScanner() {
	m_pCom->Close();
	delete[] m_pCom;
}