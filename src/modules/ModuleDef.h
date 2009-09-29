#ifndef _MODULE_DEF_H_
#define _MODULE_DEF_H_

#define MODULE_EXPORT __stdcall

// Extract progress callbacks
typedef int (CALLBACK *ExtractProgressFunc)(int, HANDLE);

struct ExtractProcessCallbacks
{
	HANDLE signalContext;
	ExtractProgressFunc Progress;
};

#define STORAGE_FORMAT_NAME_MAX_LEN 14
#define STORAGE_SUBTYPE_NAME_MAX_LEN 16

struct StorageGeneralInfo
{
	wchar_t Format[STORAGE_FORMAT_NAME_MAX_LEN];
	wchar_t SubType[STORAGE_SUBTYPE_NAME_MAX_LEN];
	DWORD NumRealItems;		// Number of items which should be queried from submodule (usually NumFiles + NumDirectories)
	DWORD NumFiles;
	DWORD NumDirectories;
	__int64 TotalSize;
};

struct ExtractOperationParams 
{
	const wchar_t* item;
	int Params;
	const wchar_t* destPath;
	ExtractProcessCallbacks Callbacks;
};

typedef int (MODULE_EXPORT *LoadSubModuleFunc)(int);
typedef int (MODULE_EXPORT *OpenStorageFunc)(const wchar_t*, INT_PTR**, StorageGeneralInfo*);
typedef void (MODULE_EXPORT *CloseStorageFunc)(INT_PTR*);
typedef int (MODULE_EXPORT *GetItemFunc)(INT_PTR*, int, LPWIN32_FIND_DATAW, wchar_t*, size_t);
typedef int (MODULE_EXPORT *ExtractFunc)(INT_PTR*, ExtractOperationParams params);

// Extract operation flags
#define SEP_ASKOVERWRITE 1

// Extract result
#define SER_SUCCESS 0
#define SER_ERROR_WRITE 1
#define SER_ERROR_READ 2
#define SER_ERROR_SYSTEM 3
#define SER_USERABORT 4

// Extract error reasons
#define EER_NOERROR 0
#define EER_READERROR 1
#define EER_WRITEERROR 2
#define EER_TARGETEXISTS 3

// Extract error reactions
#define EEN_ABORT 1
#define EEN_RETRY 2
#define EEN_SKIP 3
#define EEN_SKIPALL 4
#define EEN_CONTINUE 5
#define EEN_CONTINUESILENT 6

#endif
