#ifdef SCANNERSDK_EXPORTS
#define SCANNERSDK_API __declspec(dllexport)
#else
#define SCANNERSDK_API __declspec(dllimport)
#endif

extern "C" SCANNERSDK_API int		  openScanner(const char* portName);
extern "C" SCANNERSDK_API const char* readScanner();
extern "C" SCANNERSDK_API void		  closeScanner();