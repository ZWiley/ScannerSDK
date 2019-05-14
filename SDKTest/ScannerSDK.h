#ifdef SCANNERSDK_EXPORTS
#define SCANNERSDK_API __declspec(dllexport)
#else
#define SCANNERSDK_API __declspec(dllimport)
#endif

// �����Ǵ� ScannerSDK.dll ������
class SCANNERSDK_API CScannerSDK {
public:
	CScannerSDK(void);
	// TODO:  �ڴ�������ķ�����
};

extern "C" SCANNERSDK_API int openScanner(const char* portName);
extern "C" SCANNERSDK_API const char* readScanner();
extern "C" SCANNERSDK_API void closeScanner();