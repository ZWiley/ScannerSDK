#ifdef SCANNERSDK_EXPORTS
#define SCANNERSDK_API __declspec(dllexport)
#else
#define SCANNERSDK_API __declspec(dllimport)
#endif

// 此类是从 ScannerSDK.dll 导出的
class SCANNERSDK_API CScannerSDK {
public:
	CScannerSDK(void);
	// TODO:  在此添加您的方法。
};

extern "C" SCANNERSDK_API int openScanner(const char* portName);
extern "C" SCANNERSDK_API const char* readScanner();
extern "C" SCANNERSDK_API void closeScanner();