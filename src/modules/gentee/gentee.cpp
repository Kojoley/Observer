// gentee.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"

#include "GeaFile.h"
#include "InstallerNewFile.h"
#include "InstallerOldFile.h"

#include "gea/common/crc.h"

BaseGenteeFile* TryOpenFile(const wchar_t* path, const char* sample, size_t sampleSize)
{
	// To avoid opening of file
	if (sample && sampleSize > 3)
	{
		if ((strncmp(sample, "GEA", 3) != 0) && (strncmp(sample, "MZ", 2) != 0))
			return nullptr;
	}
	
	AStream* inFile = CFileStream::Open(path, true, false);
	if (!inFile) return nullptr;

	BaseGenteeFile* geaFile = new GeaFile();
	if (geaFile->Open(inFile))
		return geaFile;
	delete geaFile;

	BaseGenteeFile* instNewFile = new InstallerNewFile();
	if (instNewFile->Open(inFile))
		return instNewFile;
	delete instNewFile;

	BaseGenteeFile* instOldFile = new InstallerOldFile();
	if (instOldFile->Open(inFile))
		return instOldFile;
	delete instOldFile;

	delete inFile;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	BaseGenteeFile* gFile = TryOpenFile(params.FilePath, (const char*) params.Data, params.DataSize);
	if (gFile)
	{
		*storage = gFile;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, gFile->GetFileTypeName());
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, gFile->GetCompressionName());

		return SOR_SUCCESS;
	}
	
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	BaseGenteeFile* filePtr = (BaseGenteeFile*) storage;
	if (filePtr != nullptr)
	{
		filePtr->Close();
		delete filePtr;
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	BaseGenteeFile* filePtr = (BaseGenteeFile*) storage;
	if (filePtr == nullptr) return GET_ITEM_ERROR;

	if (filePtr->GetFileDesc(item_index, item_info))
		return GET_ITEM_OK;

	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	BaseGenteeFile* filePtr = (BaseGenteeFile*) storage;
	if (filePtr == nullptr) return SER_ERROR_SYSTEM;

	AStream* destStream = CFileStream::Open(params.DestPath, false, true);
	if (!destStream) return SER_ERROR_WRITE;

	GenteeExtractResult extResult = filePtr->ExtractFile(params.ItemIndex, destStream, params.Password);
	delete destStream;

	if (extResult != Success)
		DeleteFile(params.DestPath);

	switch(extResult)
	{
	case Failure:
		return SER_ERROR_SYSTEM;
	case FailedRead:
		return SER_ERROR_READ;
	case FailedWrite:
		return SER_ERROR_WRITE;
	case AbortedByUser:
		return SER_USERABORT;
	case PasswordRequired:
		return SER_PASSWORD_REQUIRED;
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
	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	crc_init();

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
