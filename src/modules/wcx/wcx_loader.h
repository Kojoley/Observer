#ifndef wcx_loader_h__
#define wcx_loader_h__

#include "wcxhead.h"

typedef HANDLE (__stdcall *OpenArchiveFunc) (tOpenArchiveData *ArchiveData);
typedef int (__stdcall *ReadHeaderFunc) (HANDLE hArcData, tHeaderData *HeaderData);
typedef int (__stdcall *ReadHeaderExFunc) (HANDLE hArcData, tHeaderDataEx *HeaderDataEx);
typedef int (__stdcall *ProcessFileFunc) (HANDLE hArcData, int Operation, char *DestPath, char *DestName);
typedef void (__stdcall *SetProcessDataProcFunc) (HANDLE hArcData, tProcessDataProc pProcessDataProc);
typedef BOOL (__stdcall *CanYouHandleThisFileFunc) (char *FileName);

typedef HANDLE (__stdcall *OpenArchiveWFunc) (tOpenArchiveDataW *ArchiveData);
typedef int (__stdcall *ReadHeaderExWFunc) (HANDLE hArcData, tHeaderDataExW *HeaderData);
typedef int (__stdcall *ProcessFileWFunc) (HANDLE hArcData, int Operation, wchar_t *DestPath, wchar_t *DestName);
typedef void (__stdcall *SetProcessDataProcWFunc) (HANDLE hArcData, tProcessDataProcW pProcessDataProc);
typedef BOOL (__stdcall *CanYouHandleThisFileWFunc) (wchar_t *FileName);

typedef int (__stdcall *CloseArchiveFunc) (HANDLE hArcData);

enum WcxModuleType
{
	WCMT_INVALID,
	WCMT_UNICODE,
	WCMT_ANSI,
	WCMT_ANSIEX
};

class WcxLoader;

class WcxModule
{
	friend class WcxLoader;

private:	
	HMODULE ModuleHandle;
	
	// Unicode functions
	CanYouHandleThisFileWFunc CanYouHandleThisFileW;
	OpenArchiveWFunc OpenArchiveW;
	ReadHeaderExWFunc ReadHeaderExW;
	ProcessFileWFunc ProcessFileW;
	SetProcessDataProcWFunc SetProcessDataProcW;

	// Ansi functions
	CanYouHandleThisFileFunc CanYouHandleThisFile;
	OpenArchiveFunc OpenArchive;
	ReadHeaderFunc ReadHeader;
	ReadHeaderExFunc ReadHeaderEx;
	ProcessFileFunc ProcessFile;
	SetProcessDataProcFunc SetProcessDataProc;

	// Common functions
	CloseArchiveFunc CloseArchive;

	WcxModuleType Type;

public:
	WcxModule() : ModuleHandle(NULL), Type(WCMT_INVALID) {}

	bool WcxIsArchive(const wchar_t* wszFilePath);

	HANDLE WcxOpenArchive(const wchar_t* wszFilePath);
	int WcxReadHeader(HANDLE hArchive);
	int WcxProcessFile(HANDLE hArchive);
	void WcsCloseArchive(HANDLE hArchive);
};

class WcxLoader
{
private:
	WcxModule* LoadSingleModule(const wchar_t* path);

public:
	std::vector<WcxModule*> Modules;

	int LoadModules(const wchar_t* basePath);
	void UnloadModules();
};

#endif // wcx_loader_h__
