// pdf.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

#include "PdfInfo.h"
#include "poppler/GlobalParams.h"

static bool optOpenAllFiles = false;

#define FILE_SIGNATURE_PDF "%PDF-"

static int DumpStringToFile(const wchar_t* destPath, const std::string &content)
{
	DWORD dwBytes;
	HANDLE hf = CreateFileW(destPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hf == INVALID_HANDLE_VALUE) return SER_ERROR_WRITE;

	int rval = SER_SUCCESS;
	if (!WriteFile(hf, content.c_str(), (DWORD) content.size(), &dwBytes, NULL))
		rval = SER_ERROR_WRITE;

	CloseHandle(hf);
	return rval;
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	if (!SignatureMatchOrNull(params.Data, params.DataSize, FILE_SIGNATURE_PDF))
		return SOR_INVALID_FILE;
	
	PDFDoc* doc = new PDFDoc(params.FilePath, (int) wcslen(params.FilePath));
	if (doc->isOk())
	{
		PdfInfo* pData = new PdfInfo();
		if (pData->LoadInfo(doc) && (optOpenAllFiles || pData->HasFiles()))
		{
			*storage = pData;

			memset(info, 0, sizeof(StorageGeneralInfo));
			swprintf_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"PDF %d.%d Document", doc->getPDFMajorVersion(), doc->getPDFMinorVersion());
			wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

			return SOR_SUCCESS;
		}
		delete pData;
	}
	delete doc;
	
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	PdfInfo* pData = (PdfInfo*) storage;
	if (pData != nullptr)
	{
		pData->Cleanup();
		delete pData;
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	PdfInfo* pData = (PdfInfo*) storage;
	if (pData == nullptr || item_index < 0) return GET_ITEM_ERROR;

	memset(item_info, 0, sizeof(StorageItemInfo));
	if (item_index == 0)
	{
		// First file will be always fake info file
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"{pdf_info}.txt");
		item_info->Size = pData->metadata.size();
		return GET_ITEM_OK;
	}
	else if (item_index < (int) pData->embFiles.size() + 1)
	{
		// File is embedded file
		
		FileSpec* fsp = pData->embFiles[item_index - 1];
		GooString* strName = fsp->getFileName();
		EmbFile* embFileInfo = fsp->getEmbeddedFile();
		
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"{files}\\");
		if (strName->hasUnicodeMarker())
		{
			int numWChars = (strName->getLength() - 2) / 2;
			size_t nextCharPos = wcslen(item_info->Path);
			char* pChar = strName->getCString() + 2;
			for (int i = 0; i < numWChars; i++)
			{
				item_info->Path[nextCharPos] = (pChar[0] << 8) | pChar[1];
				nextCharPos++;
				pChar += 2;
			}
			item_info->Path[nextCharPos] = 0;
		}
		else
		{
			size_t currLen = wcslen(item_info->Path);
			MultiByteToWideChar(CP_ACP, 0, strName->getCString(), strName->getLength(), item_info->Path + currLen, (int) (STRBUF_SIZE(item_info->Path) - currLen));
			item_info->Path[currLen + strName->getLength()] = 0;
		}
		item_info->Size = embFileInfo->size();
		TryParseDateTime(embFileInfo->createDate(), &item_info->CreationTime);
		TryParseDateTime(embFileInfo->modDate(), &item_info->ModificationTime);

		// For some special cases
		if (item_info->Size < 0) item_info->Size = 0;

		return GET_ITEM_OK;
	}
	else if (item_index < (int) (pData->embFiles.size() + pData->scripts.size() + 1))
	{
		// File is script file
		size_t scriptIndex = item_index - pData->embFiles.size() - 1;
		const PdfScriptData& scriptData = pData->scripts[scriptIndex];

		std::string scriptName(scriptData.Name);
		for (size_t i = 0; i < scriptName.length(); i++)
		{
			if (!IsValidPathChar(scriptName[i]))
				scriptName[i] = '_';
		}
		
		char fileNameBuf[MAX_PATH] = {0};
		if (scriptName.length() == 0)
			sprintf_s(fileNameBuf, "{scripts}\\script%04d", scriptIndex);
		else
			sprintf_s(fileNameBuf, "{scripts}\\script%s", scriptName.c_str());

		MultiByteToWideChar(CP_ACP, 0, fileNameBuf, -1, item_info->Path, STRBUF_SIZE(item_info->Path));
		item_info->Size = scriptData.Text.length();

		return GET_ITEM_OK;
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	PdfInfo* pData = (PdfInfo*) storage;
	if (pData == nullptr || params.ItemIndex < 0) return SER_ERROR_SYSTEM;

	if (params.ItemIndex == 0)
	{
		return DumpStringToFile(params.DestPath, pData->metadata);
	}
	else if (params.ItemIndex < (int) pData->embFiles.size() + 1)
	{
		FileSpec* fsp = pData->embFiles[params.ItemIndex - 1];
		EmbFile* embFileInfo = fsp->getEmbeddedFile();

		if (!embFileInfo->isOk())
			return SER_ERROR_READ;

		GBool ret = embFileInfo->save(params.DestPath);
		return ret ? SER_SUCCESS : SER_ERROR_WRITE;
	}
	else if (params.ItemIndex < (int) (pData->embFiles.size() + pData->scripts.size() + 1))
	{
		size_t scriptIndex = params.ItemIndex - pData->embFiles.size() - 1;
		const PdfScriptData& scriptData = pData->scripts[scriptIndex];

		return DumpStringToFile(params.DestPath, scriptData.Text);
	}
	
	return SER_ERROR_SYSTEM;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

// {993197B5-FD1B-49A6-B87F-18599F43D0AA}
static const GUID MODULE_GUID = { 0x993197b5, 0xfd1b, 0x49a6, { 0xb8, 0x7f, 0x18, 0x59, 0x9f, 0x43, 0xd0, 0xaa } };

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleId = MODULE_GUID;
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 1);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	globalParams = new GlobalParams();
	globalParams->setErrQuiet(gTrue);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
	delete globalParams;
}
