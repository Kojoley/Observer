#include "StdAfx.h"
#include "ModulesController.h"
#include "OptionsParser.h"

#define SECTION_BUF_SIZE 1024

int ModulesController::Init( const wchar_t* basePath )
{
	Cleanup();

	OptionsFile optFile(basePath);

	// Get list of modules from config file
	OptionsList *mModulesList = optFile.GetSection(L"Modules");
	if (!mModulesList) return 0;

	OptionsList *mFiltersList = optFile.GetSection(L"Filters");
	wchar_t wszModuleSection[SECTION_BUF_SIZE] = {0};

	for (size_t i = 0; i < mModulesList->NumOptions(); i++)
	{
		OptionsItem nextOpt = mModulesList->GetOption(i);
		
		ExternalModule module = {0};
		wcscpy_s(module.ModuleName, ARRAY_SIZE(module.ModuleName), nextOpt.key);
		wcscpy_s(module.LibraryFile, ARRAY_SIZE(module.LibraryFile), nextOpt.value);

		// Get module specific settings section
		memset(wszModuleSection, 0, sizeof(wszModuleSection));
		optFile.GetSectionLines(module.ModuleName, wszModuleSection, SECTION_BUF_SIZE);

		if (LoadModule(basePath, module, wszModuleSection))
		{
			// Read extensions filter for each module
			if (mFiltersList != NULL)
			{
				mFiltersList->GetValue(module.ModuleName, module.ExtensionFilter, ARRAY_SIZE(module.ExtensionFilter));
				_wcsupr_s(module.ExtensionFilter);
			}
			
			modules.push_back(module);
		}
	} // for
	
	delete mModulesList;
	if (mFiltersList) delete mFiltersList;

	return (int) modules.size();
}

void ModulesController::Cleanup()
{
	for (size_t i = 0; i < modules.size(); i++)
	{
		ExternalModule &modulePtr = modules[i];
		FreeLibrary(modulePtr.ModuleHandle);
	}
	modules.clear();
}

int ModulesController::OpenStorageFile(OpenStorageFileInParams srcParams, int *moduleIndex, HANDLE *storage, StorageGeneralInfo *sinfo)
{
	// Check input params
	if (!moduleIndex || !sinfo || !storage)
		return SOR_INVALID_FILE;

	StorageOpenParams openParams = {0};
	openParams.FilePath = srcParams.path;
	openParams.Password = srcParams.password;
	
	*moduleIndex = -1;
	for (size_t i = srcParams.startModuleIndex; i < modules.size(); i++)
	{
		const ExternalModule &modulePtr = modules[i];
		if (!srcParams.applyExtFilters || DoesExtensionFilterMatch(srcParams.path, modulePtr.ExtensionFilter))
		{
			int openRes = modulePtr.OpenStorage(openParams, storage, sinfo);
			if (openRes != SOR_INVALID_FILE)
			{
				*moduleIndex = (int) i;
				return openRes;
			}
		}
	}

	return SOR_INVALID_FILE;
}

void ModulesController::CloseStorageFile(int moduleIndex, HANDLE storage)
{
	if ((moduleIndex >= 0) && (moduleIndex < (int) modules.size()))
	{
		ExternalModule &modulePtr = modules[moduleIndex];
		modulePtr.CloseStorage(storage);
	}
}

bool ModulesController::LoadModule( const wchar_t* basePath, ExternalModule &module, const wchar_t* moduleSettings )
{
	if (!module.ModuleName[0] || !module.LibraryFile[0])
		return false;

	wchar_t wszFullModulePath[MAX_PATH] = {0};

	wcscpy_s(wszFullModulePath, MAX_PATH, basePath);
	wcscat_s(wszFullModulePath, MAX_PATH, module.LibraryFile);

	module.ModuleHandle = LoadLibraryEx(wszFullModulePath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (module.ModuleHandle != NULL)
	{
		module.LoadModule = (LoadSubModuleFunc) GetProcAddress(module.ModuleHandle, "LoadSubModule");
		module.OpenStorage = (OpenStorageFunc) GetProcAddress(module.ModuleHandle, "OpenStorage");
		module.CloseStorage = (CloseStorageFunc) GetProcAddress(module.ModuleHandle, "CloseStorage");
		module.GetNextItem = (GetItemFunc) GetProcAddress(module.ModuleHandle, "GetStorageItem");
		module.Extract = (ExtractFunc) GetProcAddress(module.ModuleHandle, "ExtractItem");

		if ((module.LoadModule != NULL) && (module.OpenStorage != NULL) &&
			(module.CloseStorage != NULL) && (module.GetNextItem != NULL) && (module.Extract != NULL))
		{
			// Try to init module
			if (module.LoadModule(moduleSettings))
				return true;
		}

		FreeLibrary(module.ModuleHandle);
		return false;
	}
	
	return false;
}

bool ModulesController::DoesExtensionFilterMatch( const wchar_t* path, const wchar_t* filter )
{
	if (!filter || !*filter)
		return true;

	const wchar_t* extPtr = wcsrchr(path, '.');
	if (extPtr != NULL && wcschr(extPtr, '\\') == NULL)
	{
		bool result = false;
		
		wchar_t* extStr = _wcsdup(extPtr);
		_wcsupr_s(extStr, wcslen(extStr)+1);

		const wchar_t* posInFilter = wcsstr(filter, extStr);
		if (posInFilter != NULL)
		{
			size_t extLen = wcslen(extStr);
			result = (posInFilter[extLen] == 0) || (posInFilter[extLen] == ';');
		}
		
		free(extStr);
		return result;
	}

	return false;
}
