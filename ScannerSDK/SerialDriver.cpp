#include	"stdafx.h"
#include	"SerialDriver.h"

#include <string>
// =========================================================
// ���캯������ʼ������˽�г�Ա
// =========================================================
SerialDriver::SerialDriver(void)
{
	mhCom = NULL;
	mRecThread = NULL;
	mbOpenFlag = FALSE;
	mStrCom = nullptr;			//���ļ��Ĵ��ں�
	muComDcb.BaudRate = 9600;	//���ļ���Ϣ
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
// ���ջ��λ������ĳ�ʼ������
// =========================================================
void	SerialDriver::CycBufInit(SerialRecBuf *pBuf)
{
	pBuf->dwRdpoint = 0;
	pBuf->dwWrpoint = 0;
	pBuf->dwFlag = CB_EMPTY;
	memset(pBuf->byRxBuf,0,RECBUFSIZE);
}

// =========================================================
// ����ԭ�ͣ�uint16_t CycBufRead(SerialRecBuf *pBuf,uint8_t *pRByte)
// �������ܣ����ջ��λ������Ķ�����
// ����˵����pBuf   ָ���λ������ṹ���ָ��
//          pRByte ���Ա���ӻ������������ֽ����ݵ�ָ��
// �������أ���ǰ��������״̬ CB_EMPTY/CB_FULL/CB_OK
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
// ����ԭ�ͣ�uint16_t CycBufWrite(SerialRecBuf *pBuf,uint8_t byWbyte)
// �������ܣ����ջ��λ�������д����
// ����˵����pBuf   ָ���λ������ṹ���ָ��
//          pRByte ���Ա���ӻ������������ֽ����ݵ�ָ��
// �������أ���ǰ��������״̬ CB_EMPTY/CB_FULL/CB_OK
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
// ����ԭ�ͣ�SetComParameter(SerialInit_t const *pSetPara)
// �������ܣ��趨���ڲ���
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
// ����ԭ�ͣ�OpenCom(void)
// �������ܣ��򿪴���
// ע��������򿪴����ļ�������ǰ��Ҫִ��SetComParameter����
//          ��ȷ�������ļ��Ĳ���������ȷ��
// =========================================================
uint16_t SerialDriver::OpenCom(void)
{
	uint16_t wErr;
	BOOL     bFlg;
	DCB      uTmpDcb;
	if(mStrCom != NULL)	//ȷ���Ѿ��Զ˿����Ƹ�ֵ
	{
		if(mhCom != NULL)
		{
			CloseHandle(mhCom);	//�رվ��
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
		if( mhCom==INVALID_HANDLE_VALUE )	//���ļ�����ʧ��
		{
			mbOpenFlag = FALSE;
			wErr = SR_FAIL;
		}
		else					//�ļ������ɹ������ö˿ڲ���
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
			if(TRUE == bFlg)	//�������óɹ��������Ѿ�׼����
			{
				mbOpenFlag = TRUE;
				wErr = StartReceiveThread();	//���������߳�
				//wErr = SR_OK;
			}
			else				//��������ʧ�ܣ�
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
// ����ԭ�ͣ�CloseCom(void)
// �������ܣ��رմ���
// =========================================================
uint16_t SerialDriver::CloseCom(void)
{
	if(mhCom!=NULL)
	{
		CloseHandle(mhCom);	//�رվ��
		mhCom = NULL;
		StopReceiveThread();
		mbOpenFlag = FALSE;	
	}
	return SR_OK;
}

// =========================================================
// ����ԭ�ͣ�GetComStatus(void)
// �������ܣ����ش��ڵ�ǰ��״̬
// =========================================================
BOOL SerialDriver::GetComStatus(void)
{
	return mbOpenFlag;
}


// =========================================================
// ����ԭ�ͣ�StartReceiveThread(void)
// �������ܣ��������������ڼ����߳�
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
	SetCommTimeouts(mhCom,&TimesOut);				//��ʱ����
	
    mRecThread = CreateThread( 
		NULL,              // default security attributes
		0,                 // use default stack size  
		SerialDriver::ReceiveThreadFunc,          // thread function 
		this,              // argument to thread function 
		0,                 // use default creation flags 
		0);   // returns the thread identifier 


	if( mRecThread==INVALID_HANDLE_VALUE )			//�̴߳������ɹ�
		wErr = SR_TREADFAIL;
	else
		wErr = SR_OK;	
	return wErr;
}

// =========================================================
// ����ԭ�ͣ�StopReceiveThread(void)
// �������ܣ�ֹͣ���ڽ��ռ����߳�
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
// ����ԭ�ͣ�DWORD ReceiveThreadFunc(LPVOID lpParam)
// �������ܣ������̵߳�������
// ����˵����
// ע�������Ա����ָ�벻�Ǻ���ָ�룬������Ҫʹ�þ�̬��Ա��������
//          ȫ�ֺ�����Ϊ�̵߳����⵫����ϣ�����������Ǿ�̬��Ա������
//          ���Խ��˺���ǿ��ת��������̴߳�����ں˺�����
// �������أ�
// =========================================================
DWORD WINAPI SerialDriver::ReceiveThreadFunc(LPVOID lpParam)
{
	SerialDriver *pObj = (SerialDriver *)lpParam;
	return pObj->ReceiveThreadKernal();
}

// =========================================================
// ����ԭ�ͣ�uint16_t ReceiveThreadKernal(void)
// �������ܣ������̴߳�����ں˺���
// ע�������Ϊ��ĳ�Ա�����������������Ա/��Ա���������Ϊ����
// �������أ�
// =========================================================
uint16_t SerialDriver::ReceiveThreadKernal(void)
{
	COMSTAT commstat;//����ṹ����Ҫ��������ȡ�˿���Ϣ��
	HANDLE  hlocEvent;
	DWORD dwError;
	DWORD dwMask;
	DWORD dwLength;
	DWORD dwByteReaded;
	DWORD i;
	uint8_t byRecBuf[1024];
	OVERLAPPED overlapped;	//OVERLAPPED�ṹ����������I/O�첽��������Բμ�MSDN

	memset(&overlapped, 0, sizeof(OVERLAPPED));	//��ʼ��OVERLAPPED����
	memset(&commstat, 0, sizeof(commstat));	
	hlocEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	//����CEvent����
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	//����CEvent����
	SetEvent(hlocEvent);	//����CEvent����Ϊ���ź�״̬
	while(FALSE!=mbOpenFlag)	//���˿ڴ��ڴ�״̬����һֱ��ȡ��Ϣ
	{		
		ClearCommError(mhCom, &dwError, &commstat);
		if(commstat.cbInQue)	//�������inbuf���н��յ����ַ���ִ������Ĳ���
		{
			WaitForSingleObject(hlocEvent, INFINITE);	//���޵ȴ�

			ResetEvent(hlocEvent);	//����CEvent����Ϊ���ź�״̬

			memset(&mosRead,0,sizeof(OVERLAPPED));
			//���Ӳ����ͨѶ�����Լ���ȡͨѶ�豸�ĵ�ǰ״̬
			ClearCommError(mhCom, &dwError, &commstat);

			dwLength = commstat.cbInQue;

			if(TRUE == ReadFile(mhCom, byRecBuf, dwLength, &dwByteReaded, &mosRead))
			{
				for(i=0;i<dwByteReaded;i++)	//���ֽڽ����յ�������д����ջ��λ�����
				{
					dwError = CycBufWrite(&mRecBuf,byRecBuf[i]);
					if(dwError==CB_FULL)	//ֱ����������д��
						break;
				}
			}
			/**/
			if(!WaitCommEvent(mhCom, &dwMask, &overlapped))
			{
				if(GetLastError()==ERROR_IO_PENDING)//�������������Ҳ��˵���ڶ�ȡ������д�����������Ĳ���
					GetOverlappedResult(mhCom,&overlapped, &dwLength, TRUE);	//���޵ȴ����I/O���������
				else
				{
					CloseHandle(overlapped.hEvent);
					return (uint16_t)-1;
				}
			}


			////����ʱ���֣���������׳���Ϣ������ClearCommError�����п���������������ѭ��
			SetEvent(hlocEvent);	//����CEvent����Ϊ���ź�״̬
			continue;
		}
		/**/
		Sleep(10);
	}

	CloseHandle(overlapped.hEvent);

	return 0;
}

// =========================================================
// ����ԭ�ͣ�uint16_t SerialReceiveByte(uint8_t *pbyChar)
// �������ܣ��ӻ������в�ѯ�Ƿ����δ����������
// ע�����
// �������أ�SR_OK/SR_FAIL �����ɹ�/ʧ��
// =========================================================
uint16_t SerialDriver::SerialReceiveByte(uint8_t *pbyChar)
{
	if(CB_OK == CycBufRead(&mRecBuf,pbyChar))
		return SR_OK;
	else
		return SR_FAIL;
}

// =========================================================
// ����ԭ�ͣ�uint16_t SerialSend(const uint8_t *pTxBuf,uint32_t dwLength)
// �������ܣ���ָ�����ȵ�����ͨ�����ڷ��ͳ�ȥ
// ����˵����pTxBuf   ���ͻ���������
//          dwLength ���ͳ���
// ע�����
// �������أ�SR_OK/SR_FAIL �����ɹ�/ʧ��
// =========================================================
uint16_t SerialDriver::SerialSend(const uint8_t *pTxBuf,uint32_t dwLength)
{
	BOOL bStatus=FALSE;
	DWORD length=0;
	COMSTAT ComStat;
	DWORD dwErr;
	if((mhCom!=INVALID_HANDLE_VALUE)&&(mbOpenFlag!=FALSE))
	{
		//ClearCommError���������Comm�еĴ��󣬴Ӷ�����������Ĵ���ͨ��GetLastErrorץȡ����
		memset(&mosWrite,0,sizeof(OVERLAPPED));
		ClearCommError(mhCom,&dwErr,&ComStat);
		bStatus=WriteFile(mhCom,pTxBuf,dwLength,&length,&mosWrite); 
		if(!bStatus)
		{
			  bStatus = GetLastError();
			  if(bStatus==ERROR_IO_PENDING)
			  {
					SetEvent(mosWrite.hEvent);
					while(!GetOverlappedResult(mhCom,&mosWrite,&length,TRUE))// �ȴ�
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
// ����ԭ�ͣ�uint16_t SerialReceive(uint8_t *pRxBuf,uint32_t dwLen,uint32_t *pRxLength)
// �������ܣ��ӽ��ջ�������ȡ��ָ�����ȵ�����
// ����˵����pRxBuf   ���ջ���������
//          dwLen    �������ճ���
//          pRxLengthʵ�ʽ��ճ���ָ��
// ע�����
// �������أ�SR_OK/SR_FAIL �����ɹ�/ʧ��
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
//	if (lReg != ERROR_SUCCESS) //�ɹ�ʱ����ERROR_SUCCESS
//	{
//		//AfxMessageBox(TEXT("��ע���ʧ�ܣ����鴮�ڲ��������\n"));
//		return FALSE;
//	}
//	lReg = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,&dwValueNumber, &MaxValueLength, NULL, NULL, NULL);
//	if (lReg != ERROR_SUCCESS) //û�гɹ�
//	{
//		//AfxMessageBox(TEXT("��ȡ��Ϣʧ�ܣ����鴮�ڲ��������\n"));
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
//			//AfxMessageBox(TEXT("ö��ϵͳ���ں�ʧ�ܣ����鴮�ڲ��������\n"));
//			return FALSE;
//		}
//		pCOMNumber = (TCHAR*)VirtualAlloc(NULL, 6, MEM_COMMIT, PAGE_READWRITE);
//		lReg = RegQueryValueEx(hKey, pValueName, NULL,NULL, (LPBYTE)pCOMNumber, &dwValueSize);
//
//		if (lReg != ERROR_SUCCESS)
//		{
//			//AfxMessageBox(TEXT("�޷���ȡϵͳ���ںţ����鴮�ڲ��������"));
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