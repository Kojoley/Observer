// msi.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "..\ModuleDef.h"
#include "MsiViewer.h"

int MODULE_EXPORT LoadSubModule(int Reserved)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	CMsiViewer *view = new CMsiViewer();
	int nOpenRes = view->Open(path);
	if (nOpenRes == ERROR_SUCCESS)
	{
		*storage = (INT_PTR *) view;

		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MSI");
		wcscpy_s(info->SubType, STORAGE_SUBTYPE_NAME_MAX_LEN, L"");
		info->NumRealItems = view->GetTotalFiles() + view->GetTotalDirectories();

		return TRUE;
	}

	delete view;
	return FALSE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (view)
		delete view;
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return FALSE;

	if (view->FindNodeDataByIndex(item_index, item_data, item_path, path_size))
		return TRUE;
	
	return FALSE;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return SER_ERROR_SYSTEM;

	FileNode *file = view->GetFile(params.item);
	if (!file) return SER_ERROR_SYSTEM;

	int nDumpResult = view->DumpFileContent(file, params.destPath);
	return nDumpResult;
}
