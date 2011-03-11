// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "WiseFile.h"

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	CWiseFile* arc = new CWiseFile();
	if (!arc->Open(params.FilePath))
	{
		delete arc;
		return SOR_INVALID_FILE;
	}

	*storage = arc;

	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Wise");
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"zlib");
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
	memset(&info->Created, 0, sizeof(info->Created));

	return SOR_SUCCESS;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	CWiseFile* arc = (CWiseFile *) storage;
	if (arc) delete arc;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	CWiseFile* arc = (CWiseFile *) storage;
	if (!arc || (item_index < 0)) return GET_ITEM_ERROR;

	WiseFileRec fileData = {0};
	bool noMoreItems = false;
	if (arc->GetFileInfo(item_index, &fileData, noMoreItems))
	{
		if (fileData.FileName[0])
			wcscpy_s(item_path, path_size, fileData.FileName);
		else
			swprintf_s(item_path, path_size, L"file%04d.bin", item_index);
		
		memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
		
		wchar_t* wszSlash = wcsrchr(item_path, '\\');
		if (wszSlash)
			wcscpy_s(item_data->cFileName, MAX_PATH, wszSlash + 1);
		else
			wcscpy_s(item_data->cFileName, MAX_PATH, item_path);
		item_data->nFileSizeLow = fileData.UnpackedSize;

		return GET_ITEM_OK;
	}

	return (noMoreItems) ? GET_ITEM_NOMOREITEMS : GET_ITEM_ERROR;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	CWiseFile* arc = (CWiseFile *) storage;
	if (!arc) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= arc->NumFiles())
		return SER_ERROR_READ;

	bool rval = arc->ExtractFile(params.item, params.destFilePath);
	return rval ? SER_SUCCESS : SER_ERROR_WRITE;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 0);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->OpenStorage = OpenStorage;
	LoadParams->CloseStorage = CloseStorage;
	LoadParams->GetItem = GetStorageItem;
	LoadParams->ExtractItem = ExtractItem;

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
