#if _MSC_VER > 1800

#ifndef	__SerialDriver_H__
#define __SerialDriver_H__

#define RECBUFSIZE	1024
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

enum SerialErr
{
	SR_OK   = 0x0000,
	SR_FAIL = 0xA000,
	SR_PARAERR = 0xA001,
	SR_TIMEOUT = 0xA002,
	SR_TREADFAIL = 0xA003,
	SR_COMCLS = 0xA004,
};
enum BufStatus
{
	CB_OK = 0x0,
	CB_EMPTY = 0x1,
	CB_FULL = 0x2,
	CB_INIT = 0xFFFFFFFF,
};
typedef struct	_SerialInit_
{
	const char* strCom;
	DWORD   dwBaud;
	BYTE	ByteSize;
	BYTE	Parity;
	BYTE	StopBits;
}SerialInit_t;

typedef struct _SerialRecBuf_
{
	uint32_t dwWrpoint;
	uint32_t dwRdpoint;
	uint32_t dwFlag;
	uint8_t	 byRxBuf[RECBUFSIZE];
}SerialRecBuf;

class SerialDriver
{
private:
	HANDLE		mhCom;
	HANDLE		mRecThread;
	//HANDLE	mhPostMsgEvent;			//事件句柄
	BOOL		mbOpenFlag;
	OVERLAPPED	mosRead;				//接收数据状态
	OVERLAPPED	mosWrite;				//发送数据状态
	const char*	mStrCom;				//打开文件的串口号
	DCB			muComDcb;				//打开文件信息
	SerialRecBuf mRecBuf;
	uint16_t	ReceiveThreadKernal(void);
	void	    CycBufInit(SerialRecBuf *pBuf);
	uint16_t	CycBufRead(SerialRecBuf *pBuf,uint8_t *pRByte);
	uint16_t	CycBufWrite(SerialRecBuf *pBuf,uint8_t byWbyte);
public:
	SerialDriver(void);
	uint16_t SetComParameter(SerialInit_t const *pSetPara);
	uint16_t OpenCom(void);
	uint16_t CloseCom(void);
	BOOL     GetComStatus(void);
	uint16_t SerialSend(uint8_t const *pTxBuf,uint32_t dwLength);
	uint16_t StartReceiveThread(void);
	uint16_t StopReceiveThread(void);
	static DWORD WINAPI ReceiveThreadFunc(LPVOID lpParam);	//线程函数，必须使用静态成员函数或全局函数	
	uint16_t SerialReceiveByte(uint8_t *pbyChar);
	uint16_t SerialReceive(uint8_t *pRxBuf,uint32_t dwLen,uint32_t *pRxLength);
	//bool SerialGetPort(CStringList* port);
};

#endif

#endif
