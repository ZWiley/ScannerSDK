#ifdef YIRUISCANNER_EXPORTS
#define YIRUISCANNER_API __declspec(dllexport)
#else
#define YIRUISCANNER_API __declspec(dllimport)
#endif

extern "C" YIRUISCANNER_API int		    openScanner(const char* portName);
extern "C" YIRUISCANNER_API const char* readScanner();
extern "C" YIRUISCANNER_API void	    closeScanner();