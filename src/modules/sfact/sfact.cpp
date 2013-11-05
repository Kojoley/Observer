// sfact.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "SfFile.h"

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	SetupFactoryFile* sfInst = OpenInstaller(params.FilePath);
	if (sfInst != nullptr)
	{
		*storage = sfInst;

		memset(info, 0, sizeof(StorageGeneralInfo));
		swprintf_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Setup Factory %d Installer", sfInst->GetVersion());
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");
		
		return SOR_SUCCESS;
	}

	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	SetupFactoryFile* sfInst = (SetupFactoryFile*) storage;
	if (sfInst != NULL)
	{
		FreeInstaller(sfInst);
	}
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	SetupFactoryFile* sfInst = (SetupFactoryFile*) storage;
	if (sfInst == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (sfInst->GetCount() == 0)
	{
		if (sfInst->EnumFiles() < 0)
			return GET_ITEM_ERROR;
	}
	
	if (item_index < (int) sfInst->GetCount())
	{
		SFFileEntry fe = sfInst->GetFile(item_index);
		
		memset(item_info, 0, sizeof(StorageItemInfo));
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), fe.LocalPath);
		item_info->Size = fe.UnpackedSize;
		
		return GET_ITEM_OK;
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	SetupFactoryFile* sfInst = (SetupFactoryFile*) storage;
	if (sfInst == NULL || params.item < 0 || params.item >= (int) sfInst->GetCount()) return SER_ERROR_SYSTEM;

	CFileStream* outStream = new CFileStream(params.destFilePath, false, true);
	if (!outStream->IsValid())
	{
		delete outStream;
		return SER_ERROR_WRITE;
	}
	
	bool extractResult = sfInst->ExtractFile(params.item, outStream);
	delete outStream;

	if (!extractResult)
	{
		DeleteFile(params.destFilePath);
		return SER_ERROR_READ;
	}
	
	return SER_SUCCESS;
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
