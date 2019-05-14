#include	"stdafx.h"
#include	"SerialDriver.h"

#include <string>
// =========================================================
// 构造函数：初始化部分私有成员
// =========================================================
SerialDriver::SerialDriver(void)
{
	mhCom = NULL;
	mRecThread = NULL;
	mbOpenFlag = FALSE;
	mStrCom = nullptr;			//打开文件的串口号
	muComDcb.BaudRate = 9600;	//打开文件信息
	muComDcb.ByteSize = 8;
	muComDcb.StopBits = ONESTOPBIT;
	muComDcb.fDtrControl = FALSE;
	muComDcb.Parity = FALSE;
	muComDcb.fBinary = TRUE;
	//muComDcb.XonChar = 0x11;
	//muComDcb.XoffChar = 0x13;
	CycBufInit(&mRecBuf);
}

// =========================================================
// 接收环形缓冲区的初始化函数
// =========================================================
void	SerialDriver::CycBufInit(SerialRecBuf *pBuf)
{
	pBuf->dwRdpoint = 0;
	pBuf->dwWrpoint = 0;
	pBuf->dwFlag = CB_EMPTY;
	memset(pBuf->byRxBuf,0,RECBUFSIZE);
}

// =========================================================
// 函数原型：uint16_t CycBufRead(SerialRecBuf *pBuf,uint8_t *pRByte)
// 函数功能：接收环形缓冲区的读数据
// 参数说明：pBuf   指向环形缓冲区结构体的指针
//          pRByte 用以保存从缓冲区读出的字节数据的指针
// 函数返回：当前缓冲区的状态 CB_EMPTY/CB_FULL/CB_OK
// =========================================================
uint16_t	SerialDriver::CycBufRead(SerialRecBuf *pBuf,uint8_t *pRByte)
{
	if((pBuf->dwFlag != CB_FULL)&&(pBuf->dwRdpoint==pBuf->dwWrpoint))
		pBuf->dwFlag = CB_EMPTY;
	else if(pBuf->dwFlag != CB_EMPTY)
	{
		*pRByte = pBuf->byRxBuf[pBuf->dwRdpoint];
		pBuf->dwRdpoint++;
		if(pBuf->dwRdpoint>=RECBUFSIZE)
			pBuf->dwRdpoint = 0;
		pBuf->dwFlag = CB_OK;
	}
	return (uint16_t)pBuf->dwFlag;
}

// =========================================================
// 函数原型：uint16_t CycBufWrite(SerialRecBuf *pBuf,uint8_t byWbyte)
// 函数功能：接收环形缓冲区的写数据
// 参数说明：pBuf   指向环形缓冲区结构体的指针
//          pRByte 用以保存从缓冲区读出的字节数据的指针
// 函数返回：当前缓冲区的状态 CB_EMPTY/CB_FULL/CB_OK
// =========================================================
uint16_t	SerialDriver::CycBufWrite(SerialRecBuf *pBuf,uint8_t byWbyte)
{
	if((pBuf->dwFlag != CB_EMPTY)&&(pBuf->dwRdpoint==pBuf->dwWrpoint))
		pBuf->dwFlag = CB_FULL;
	else if(pBuf->dwFlag != CB_FULL)
	{
		pBuf->byRxBuf[pBuf->dwWrpoint] = byWbyte;
		pBuf->dwWrpoint++;
		if(pBuf->dwWrpoint>=RECBUFSIZE)
			pBuf->dwWrpoint = 0;
		pBuf->dwFlag = CB_OK;
	}
	return (uint16_t)pBuf->dwFlag;
}

// =========================================================
// 函数原型：SetComParameter(SerialInit_t const *pSetPara)
// 函数功能：设定串口参数
// =========================================================
uint16_t SerialDriver::SetComParameter(SerialInit_t const *pSetPara)
{
	mStrCom = pSetPara->strCom;
	muComDcb.BaudRate = pSetPara->dwBaud;
	muComDcb.ByteSize = pSetPara->ByteSize;
	muComDcb.Parity = pSetPara->Parity;
	muComDcb.StopBits = pSetPara->StopBits;
	return 0;
}

// =========================================================
// 函数原型：OpenCom(void)
// 函数功能：打开串口
// 注意事项：仅打开串口文件，调用前需要执行SetComParameter函数
//          以确保串口文件的参数设置正确。
// =========================================================
uint16_t SerialDriver::OpenCom(void)
{
	uint16_t wErr;
	BOOL     bFlg;
	DCB      uTmpDcb;
	if(mStrCom != NULL)	//确定已经对端口名称赋值
	{
		if(mhCom != NULL)
		{
			CloseHandle(mhCom);	//关闭句柄
			mbOpenFlag = FALSE;	
		}
		mhCom = ::CreateFileA(	
							((std::string("\\\\?\\") + std::string(mStrCom)).c_str()),
							GENERIC_READ|GENERIC_WRITE,
							0,
							NULL,
							OPEN_EXISTING, 
							FILE_FLAG_OVERLAPPED,
							NULL
						   );
		if( mhCom==INVALID_HANDLE_VALUE )	//若文件创建失败
		{
			mbOpenFlag = FALSE;
			wErr = SR_FAIL;
		}
		else					//文件创建成功，设置端口参数
		{
			bFlg = GetCommState(mhCom,&uTmpDcb);
			uTmpDcb.BaudRate = muComDcb.BaudRate;
			uTmpDcb.ByteSize = muComDcb.ByteSize;
			uTmpDcb.Parity = muComDcb.Parity;
			uTmpDcb.StopBits = muComDcb.StopBits;
			uTmpDcb.fBinary = muComDcb.fBinary;
			uTmpDcb.fParity = muComDcb.fParity;
			uTmpDcb.XonChar = muComDcb.XonChar;
			uTmpDcb.XoffChar = muComDcb.XoffChar;
			bFlg = SetCommState(mhCom,&uTmpDcb);	
			if(TRUE == bFlg)	//参数设置成功，串口已经准备好
			{
				mbOpenFlag = TRUE;
				wErr = StartReceiveThread();	//开启监听线程
				//wErr = SR_OK;
			}
			else				//参数设置失败！
			{
				mbOpenFlag = FALSE;
				wErr = SR_FAIL;
			}
		}
	}
	else
		wErr = SR_PARAERR;

	return wErr;
}

// =========================================================
// 函数原型：CloseCom(void)
// 函数功能：关闭串口
// =========================================================
uint16_t SerialDriver::CloseCom(void)
{
	if(mhCom!=NULL)
	{
		CloseHandle(mhCom);	//关闭句柄
		mhCom = NULL;
		StopReceiveThread();
		mbOpenFlag = FALSE;	
	}
	return SR_OK;
}

// =========================================================
// 函数原型：GetComStatus(void)
// 函数功能：返回串口当前打开状态
// =========================================================
BOOL SerialDriver::GetComStatus(void)
{
	return mbOpenFlag;
}


// =========================================================
// 函数原型：StartReceiveThread(void)
// 函数功能：创建并启动串口监听线程
// =========================================================
uint16_t SerialDriver::StartReceiveThread(void)
{
	COMMTIMEOUTS	TimesOut;
	uint16_t wErr;
	SetCommMask(mhCom, EV_RXCHAR);
	SetupComm(mhCom, 1024, 1024);
	TimesOut.ReadIntervalTimeout = MAXDWORD;
	TimesOut.ReadTotalTimeoutConstant = 0;
	TimesOut.ReadTotalTimeoutMultiplier = 0;
	TimesOut.WriteTotalTimeoutConstant = 50;
	TimesOut.WriteTotalTimeoutMultiplier = 2000;
	SetCommTimeouts(mhCom,&TimesOut);				//超时设置
	
    mRecThread = CreateThread( 
		NULL,              // default security attributes
		0,                 // use default stack size  
		SerialDriver::ReceiveThreadFunc,          // thread function 
		this,              // argument to thread function 
		0,                 // use default creation flags 
		0);   // returns the thread identifier 


	if( mRecThread==INVALID_HANDLE_VALUE )			//线程创建不成功
		wErr = SR_TREADFAIL;
	else
		wErr = SR_OK;	
	return wErr;
}

// =========================================================
// 函数原型：StopReceiveThread(void)
// 函数功能：停止串口接收监听线程
// =========================================================
uint16_t SerialDriver::StopReceiveThread(void)
{
	uint16_t wErr;	
	if( mRecThread!=INVALID_HANDLE_VALUE )
	{
		//AfxEndThread();		
		CloseHandle(mRecThread);
	}
	mRecThread=NULL;
	wErr = SR_OK;
	return wErr;
}

// =========================================================
// 函数原型：DWORD ReceiveThreadFunc(LPVOID lpParam)
// 函数功能：监听线程的主函数
// 参数说明：
// 注意事项：成员函数指针不是函数指针，所以需要使用静态成员函数或者
//          全局函数作为线程的主题但是又希望调用其他非静态成员函数，
//          所以将此函数强制转换后调用线程处理的内核函数。
// 函数返回：
// =========================================================
DWORD WINAPI SerialDriver::ReceiveThreadFunc(LPVOID lpParam)
{
	SerialDriver *pObj = (SerialDriver *)lpParam;
	return pObj->ReceiveThreadKernal();
}

// =========================================================
// 函数原型：uint16_t ReceiveThreadKernal(void)
// 函数功能：监听线程处理的内核函数
// 注意事项：作为类的成员函数，对类的其他成员/成员函数处理较为方便
// 函数返回：
// =========================================================
uint16_t SerialDriver::ReceiveThreadKernal(void)
{
	COMSTAT commstat;//这个结构体主要是用来获取端口信息的
	HANDLE  hlocEvent;
	DWORD dwError;
	DWORD dwMask;
	DWORD dwLength;
	DWORD dwByteReaded;
	DWORD i;
	uint8_t byRecBuf[1024];
	OVERLAPPED overlapped;	//OVERLAPPED结构体用来设置I/O异步，具体可以参见MSDN

	memset(&overlapped, 0, sizeof(OVERLAPPED));	//初始化OVERLAPPED对象
	memset(&commstat, 0, sizeof(commstat));	
	hlocEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	//创建CEvent对象
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	//创建CEvent对象
	SetEvent(hlocEvent);	//设置CEvent对象为无信号状态
	while(FALSE!=mbOpenFlag)	//若端口处于打开状态，则一直读取消息
	{		
		ClearCommError(mhCom, &dwError, &commstat);
		if(commstat.cbInQue)	//如果串口inbuf中有接收到的字符就执行下面的操作
		{
			WaitForSingleObject(hlocEvent, INFINITE);	//无限等待

			ResetEvent(hlocEvent);	//设置CEvent对象为无信号状态

			memset(&mosRead,0,sizeof(OVERLAPPED));
			//清除硬件的通讯错误以及获取通讯设备的当前状态
			ClearCommError(mhCom, &dwError, &commstat);

			dwLength = commstat.cbInQue;

			if(TRUE == ReadFile(mhCom, byRecBuf, dwLength, &dwByteReaded, &mosRead))
			{
				for(i=0;i<dwByteReaded;i++)	//按字节将接收到的数据写入接收环形缓冲区
				{
					dwError = CycBufWrite(&mRecBuf,byRecBuf[i]);
					if(dwError==CB_FULL)	//直到缓冲区被写满
						break;
				}
			}
			/**/
			if(!WaitCommEvent(mhCom, &dwMask, &overlapped))
			{
				if(GetLastError()==ERROR_IO_PENDING)//如果操作被挂起，也就说正在读取或正在写，则进行下面的操作
					GetOverlappedResult(mhCom,&overlapped, &dwLength, TRUE);	//无限等待这个I/O操作的完成
				else
				{
					CloseHandle(overlapped.hEvent);
					return (uint16_t)-1;
				}
			}


			////测试时发现，当如果在抛出消息后马上ClearCommError，将有可能引起程序进入死循环
			SetEvent(hlocEvent);	//设置CEvent对象为无信号状态
			continue;
		}
		/**/
		Sleep(10);
	}

	CloseHandle(overlapped.hEvent);

	return 0;
}

// =========================================================
// 函数原型：uint16_t SerialReceiveByte(uint8_t *pbyChar)
// 函数功能：从缓冲区中查询是否存在未读出的数据
// 注意事项：
// 函数返回：SR_OK/SR_FAIL 读出成功/失败
// =========================================================
uint16_t SerialDriver::SerialReceiveByte(uint8_t *pbyChar)
{
	if(CB_OK == CycBufRead(&mRecBuf,pbyChar))
		return SR_OK;
	else
		return SR_FAIL;
}

// =========================================================
// 函数原型：uint16_t SerialSend(const uint8_t *pTxBuf,uint32_t dwLength)
// 函数功能：将指定长度的数据通过串口发送出去
// 参数说明：pTxBuf   发送缓冲区数组
//          dwLength 发送长度
// 注意事项：
// 函数返回：SR_OK/SR_FAIL 读出成功/失败
// =========================================================
uint16_t SerialDriver::SerialSend(const uint8_t *pTxBuf,uint32_t dwLength)
{
	BOOL bStatus=FALSE;
	DWORD length=0;
	COMSTAT ComStat;
	DWORD dwErr;
	if((mhCom!=INVALID_HANDLE_VALUE)&&(mbOpenFlag!=FALSE))
	{
		//ClearCommError是用来清除Comm中的错误，从而可以在下面的代码通过GetLastError抓取错误
		memset(&mosWrite,0,sizeof(OVERLAPPED));
		ClearCommError(mhCom,&dwErr,&ComStat);
		bStatus=WriteFile(mhCom,pTxBuf,dwLength,&length,&mosWrite); 
		if(!bStatus)
		{
			  bStatus = GetLastError();
			  if(bStatus==ERROR_IO_PENDING)
			  {
					SetEvent(mosWrite.hEvent);
					while(!GetOverlappedResult(mhCom,&mosWrite,&length,TRUE))// 等待
					{
						if(GetLastError()==ERROR_IO_INCOMPLETE)
						   continue;
					}
			  }
			  else
				return SR_FAIL;
		}
		return SR_OK;
	}
	else
		return SR_COMCLS;

}

// =========================================================
// 函数原型：uint16_t SerialReceive(uint8_t *pRxBuf,uint32_t dwLen,uint32_t *pRxLength)
// 函数功能：从接收缓冲区中取出指定长度的数据
// 参数说明：pRxBuf   接收缓冲区数组
//          dwLen    期望接收长度
//          pRxLength实际接收长度指针
// 注意事项：
// 函数返回：SR_OK/SR_FAIL 读出成功/失败
// =========================================================
uint16_t SerialDriver::SerialReceive(uint8_t *pRxBuf,uint32_t dwLen,uint32_t *pRxLength)
{
	uint32_t i;
	for(i=0;i<dwLen;i++)
	{
		if(SR_OK != SerialReceiveByte(&pRxBuf[i]))
			break;
	}
	*pRxLength = i;
	return SR_OK;
}
//bool SerialDriver::SerialGetPort(CStringList* port)
//{
//	long lReg;
//	HKEY hKey;
//	DWORD MaxValueLength;
//	DWORD dwValueNumber;
//	lReg = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"),0, KEY_QUERY_VALUE, &hKey);
//	if (lReg != ERROR_SUCCESS) //成功时返回ERROR_SUCCESS
//	{
//		//AfxMessageBox(TEXT("打开注册表失败，请检查串口并重启软件\n"));
//		return FALSE;
//	}
//	lReg = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,&dwValueNumber, &MaxValueLength, NULL, NULL, NULL);
//	if (lReg != ERROR_SUCCESS) //没有成功
//	{
//		//AfxMessageBox(TEXT("获取信息失败，请检查串口并重启软件\n"));
//		return FALSE;
//	}
//	TCHAR *pValueName, *pCOMNumber;
//	DWORD cchValueName, dwValueSize = 10;
//	for (int i = 0; i < dwValueNumber; i++)
//	{
//		cchValueName = MaxValueLength + 1;
//		dwValueSize = 10;
//		pValueName = (TCHAR*)VirtualAlloc(NULL, cchValueName, MEM_COMMIT, PAGE_READWRITE);
//		lReg = RegEnumValue(hKey, i, pValueName, &cchValueName, NULL, NULL, NULL, NULL);
//
//		if ((lReg != ERROR_SUCCESS) && (lReg != ERROR_NO_MORE_ITEMS))
//		{
//			//AfxMessageBox(TEXT("枚举系统串口号失败，请检查串口并重启软件\n"));
//			return FALSE;
//		}
//		pCOMNumber = (TCHAR*)VirtualAlloc(NULL, 6, MEM_COMMIT, PAGE_READWRITE);
//		lReg = RegQueryValueEx(hKey, pValueName, NULL,NULL, (LPBYTE)pCOMNumber, &dwValueSize);
//
//		if (lReg != ERROR_SUCCESS)
//		{
//			//AfxMessageBox(TEXT("无法获取系统串口号，请检查串口并重启软件"));
//			return FALSE;
//		}
//
//		const char* str(pCOMNumber);
//		port->AddTail(str);
//
//		VirtualFree(pValueName, 0, MEM_RELEASE);
//		VirtualFree(pCOMNumber, 0, MEM_RELEASE);
//	}
//	return TRUE;
//}