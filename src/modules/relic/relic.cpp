// relic.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"

#include "HWClassFactory.h"

static const wchar_t* GetFileName(const wchar_t* fullPath)
{
	const wchar_t* lastSlash = wcsrchr(fullPath, '\\');
	if (lastSlash)
		return lastSlash + 1;
	else
		return fullPath;
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	CHWAbstractStorage* fileObj = CHWClassFactory::LoadFile(path);
	if (fileObj != NULL)
	{
		*storage = (INT_PTR*) fileObj;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, fileObj->GetFormatName());
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, fileObj->GetLabel());
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, fileObj->GetCompression());

		return TRUE;
	}

	return FALSE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	if (storage)
	{
		CHWAbstractStorage* fileObj = (CHWAbstractStorage*) storage;
		delete fileObj;
	}
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	CHWAbstractStorage* fileObj = (CHWAbstractStorage*) storage;
	if (!fileObj) return GET_ITEM_ERROR;
	
	if (item_index < 0) return GET_ITEM_ERROR;
	if (item_index >= fileObj->NumFiles()) return GET_ITEM_NOMOREITEMS;

	HWStorageItem item = {0};
	if (fileObj->GetFileInfo(item_index, &item))
	{
		wcscpy_s(item_path, path_size, item.Name);

		memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
		wcscpy_s(item_data->cFileName, MAX_PATH, GetFileName(item.Name));
		wcscpy_s(item_data->cAlternateFileName, 14, L"");
		item_data->dwFileAttributes = 0;
		item_data->nFileSizeLow = item.UncompressedSize;
		item_data->ftLastWriteTime = item.ModTime;

		return GET_ITEM_OK;
	}
	
	return GET_ITEM_ERROR;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	CHWAbstractStorage* fileObj = (CHWAbstractStorage*) storage;
	if (!fileObj) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= fileObj->NumFiles())
		return SER_ERROR_SYSTEM;

	HWStorageItem item = {0};
	if (fileObj->GetFileInfo(params.item, &item))
	{
		HANDLE hOutputFile = CreateFile(params.destFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hOutputFile == INVALID_HANDLE_VALUE)
			return SER_ERROR_WRITE;

		bool fOpRes = fileObj->ExtractFile(params.item, hOutputFile);

		if (fOpRes && (item.ModTime.dwLowDateTime || item.ModTime.dwHighDateTime))
			SetFileTime(hOutputFile, NULL, NULL, &item.ModTime);

		CloseHandle(hOutputFile);

		if (!fOpRes)
		{
			DeleteFile(params.destFilePath);
			return SER_ERROR_READ;
		}

		return SER_SUCCESS;
	}

	return SER_ERROR_SYSTEM;
}
