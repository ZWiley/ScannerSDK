#include "stdafx.h"

#include "WinCom.h"
#include <thread>
using namespace std;

#define  MAX_BUFF 2048

WinCom::WinCom()
{
	memset(&osRead, 0, sizeof(osRead));
	memset(&osWrite, 0, sizeof(osWrite));
	memset(&ShareEvent, 0, sizeof(ShareEvent));
	COMFile = NULL;
}

WinCom::~WinCom()
{
	Close();
}

BOOL WinCom::Open(const char* com_name, int baudRate)//szCom.Format(_T("\\\\.\\COM%d"), nPort);
{
	is_com_close = false;
	osRead.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (osRead.hEvent == NULL) {
		return FALSE;
	}

	osWrite.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (osWrite.hEvent == NULL) {
		return FALSE;
	}

	ShareEvent.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (ShareEvent.hEvent == NULL) {
		return FALSE;
	}

	COMFile = CreateFile(com_name, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, 
		OPEN_EXISTING, 
		FILE_FLAG_OVERLAPPED, 
		NULL
	);

	int nError = GetLastError();
	if (INVALID_HANDLE_VALUE == COMFile) {
		char chError[256]; memset(chError, 0, 256);
		int nBuffLen = 256;
		return FALSE;
	}
	SetupComm(COMFile, 4096, 4096);
	SetCommMask(COMFile, EV_RXCHAR);

	COMMTIMEOUTS CommTimeOuts;
	CommTimeOuts.ReadIntervalTimeout = MAXDWORD;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(COMFile, &CommTimeOuts);

	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(COMFile, &dcb);
	dcb.BaudRate = baudRate;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity   = NOPARITY;
	dcb.ByteSize = 8;
	dcb.fBinary  = TRUE;//二进制通信, 非字符通信

	BOOL fRetVal = SetCommState(COMFile, &dcb);
	if (!fRetVal) return FALSE;

	//刷清缓冲区
	PurgeComm(COMFile, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	//指定串口执行扩展功能
	EscapeCommFunction(COMFile, SETDTR);

	//启动接受线程
	thread th([=] {
		while (is_com_close== false) {
			DWORD nResutl = WaitForSingleObjectEx(osRead.hEvent, 5, TRUE/*INFINITE*/);
			if (nResutl == WAIT_OBJECT_0)
			{
				//  			WORD nLen = (WORD)m_nBuffLen + 2;
				//  			PBYTE pIn = new BYTE[nLen];
				//  			pIn[0] = HIBYTE(nLen);
				//  			pIn[1] = LOBYTE(nLen);
				//  			memcpy(pIn + 2, m_InPutBuff, m_nBuffLen);
				// 			
				// 			CString str; str.Format("revv:%d", m_nBuffLen);
				// 			OutputDebugString(str);
				// 
				//  			g_InPutList.AddTail(pIn);
				//  			m_nBuffLen = 0;
			}

			DWORD dwEvtMask = 0;
			WaitCommEvent(COMFile, &dwEvtMask, &ShareEvent);//等待串口事件
			if ((dwEvtMask & EV_RXCHAR) == EV_RXCHAR) {
				Read();
			}
		}

		//子线程结束
		OutputDebugStringA("recv data thread exit.");
	});

	th.detach();

	//启动数据处理线程
	thread th_data([=] {
		while (is_com_close == false) {
		
			data_info* pGet = nullptr;
			if (m_data_queue.try_pop(pGet) == false)
				this_thread::sleep_for(chrono::milliseconds(10));
			else 
			{
				if(m_fn) m_fn(pGet);
			}
		}

		OutputDebugStringA("data handle thread exit.");
	});

	th_data.detach();

	return TRUE;
}


void WinCom::Close()
{
	if (COMFile != nullptr) 
	{
		is_com_close = true;
		SetCommMask(COMFile, 0);

		EscapeCommFunction(COMFile, CLRDTR);

		PurgeComm(COMFile, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

		CloseHandle(COMFile);
		COMFile = nullptr;

		CloseHandle(osRead.hEvent);
		memset(&osRead, 0, sizeof(osRead));;
		CloseHandle(osWrite.hEvent);
		memset(&osWrite, 0, sizeof(osWrite));
		CloseHandle(ShareEvent.hEvent);
		memset(&ShareEvent, 0, sizeof(ShareEvent));

		
		data_info* pdata = nullptr;
		while (m_data_queue.try_pop(pdata))
		{
			delete pdata;
			pdata = nullptr;
		}
		m_data_queue.empty();
		
	}
	
	return ;
}


const char*  WinCom::Read()
{
    char m_InPutBuff[MAX_BUFF] = {0};

	DWORD dwErrorFlags;
	COMSTAT ComStat ;
	DWORD dwLength;
	DWORD dwOutBytes = 0;
	ClearCommError( COMFile, &dwErrorFlags, &ComStat ) ;
	dwLength = ComStat.cbInQue;

	if (dwLength > MAX_BUFF)
		dwLength = MAX_BUFF;
	
	if (dwLength > 0)
	{
		BOOL bret = ReadFile(COMFile, m_InPutBuff, dwLength, &dwOutBytes, &osRead);
		if (bret == FALSE && GetLastError() == ERROR_IO_PENDING) 
		{
			DWORD bRecvIncache = 0;
			GetOverlappedResult(COMFile, &osRead, &bRecvIncache, TRUE);
			WORD nLen = (WORD)bRecvIncache + 2;

			data_info* p_new = new data_info(m_InPutBuff, dwLength);
			m_data_queue.push(p_new);
		}
	}
		
	return m_InPutBuff;
}


int WinCom::Write(PBYTE pData, int nDataLen)
{
	if (NULL == pData || nDataLen < 1)
		return FALSE;
	DWORD nLen = 0;
	if (! WriteFile(COMFile, (LPCVOID)pData, nDataLen, &nLen, &/*ShareEvent*/osWrite)) {

		if (WAIT_OBJECT_0 == WaitForSingleObject(osWrite.hEvent, 0xFFFFFF)) 
			ResetEvent(osWrite.hEvent);
		else
			ResetEvent(osWrite.hEvent);

		DWORD nError = GetLastError();
		if (ERROR_IO_PENDING != nError) {//ERROR_IO_PENDING异步没完成
			char chError[256];memset(chError, 0, 256);
			int nBuffLen = 256;
			return -1;
		}
	}

	return 0;
}




