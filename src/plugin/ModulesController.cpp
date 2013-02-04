#include "StdAfx.h"
#include "ModulesController.h"

#define SECTION_BUF_SIZE 1024

static bool DoesExtensionFilterMatch( const wchar_t* path, const vector<wstring> &filter )
{
	if (filter.size() == 0)
		return true;

	for (size_t i = 0; i < filter.size(); i++)
	{
		if (PathMatchSpec(path, filter[i].c_str()))
			return true;
	}

	return false;
}

static void ParseExtensionList(const wchar_t* listval, ExternalModule& module)
{
	if (!listval || !listval[0]) return;
	
	wchar_t* context = NULL;
	wchar_t* strList = _wcsdup(listval);

	wchar_t* token = wcstok_s(strList, L";", &context);
	while (token)
	{
		if (wcslen(token) > 0)
		{
			while(!token[0]) token++;
			module.ExtensionFilter.push_back(token);
		}

		token = wcstok_s(NULL, L";", &context);
	}
	free(strList);
}

int ModulesController::Init( const wchar_t* basePath, Config* cfg )
{
	Cleanup();

	// Get list of modules from config file
	ConfigSection *mModulesList = cfg->GetSection(L"Modules");
	if (!mModulesList) return 0;

	ConfigSection *mFiltersList = cfg->GetSection(L"Filters");

	for (size_t i = 0; i < mModulesList->Count(); i++)
	{
		const ConfigItem& nextOpt = mModulesList->GetItem(i);
		ExternalModule module(nextOpt.Key.c_str(), nextOpt.Value.c_str());
		
		ConfigSection* moduleOpts = cfg->GetSection(module.Name());
		wstring moduleSettingsStr = moduleOpts ? moduleOpts->GetAll() : L"";

		if (LoadModule(basePath, module, moduleSettingsStr.c_str()))
		{
			// Read extensions filter for each module
			if (mFiltersList != NULL)
			{
				wchar_t extList[1024] = {0};
				mFiltersList->GetValue(module.Name(), extList, ARRAY_SIZE(extList));
				ParseExtensionList(extList, module);
			}
			
			modules.push_back(module);
		}
	} // for
	
	return (int) modules.size();
}

void ModulesController::Cleanup()
{
	for (size_t i = 0; i < modules.size(); i++)
	{
		ExternalModule &modulePtr = modules[i];
		modulePtr.UnloadModule();
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
	for (size_t i = 0; i < modules.size(); i++)
	{
		if (srcParams.openWithModule == -1 || srcParams.openWithModule == i)
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
	} // for i

	return SOR_INVALID_FILE;
}

void ModulesController::CloseStorageFile(int moduleIndex, HANDLE storage)
{
	if ((moduleIndex >= 0) && (moduleIndex < (int) modules.size()))
	{
		const ExternalModule &modulePtr = modules[moduleIndex];
		modulePtr.CloseStorage(storage);
	}
}

bool ModulesController::LoadModule( const wchar_t* basePath, ExternalModule &module, const wchar_t* moduleSettings )
{
	if (!module.Name()[0] || !module.LibraryFile()[0])
		return false;

	wchar_t wszFullModulePath[MAX_PATH] = {0};
	swprintf_s(wszFullModulePath, MAX_PATH, L"%s%s", basePath, module.LibraryFile());

	module.ModuleHandle = LoadLibraryEx(wszFullModulePath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (module.ModuleHandle != NULL)
	{
		// Search for exported functions
		module.LoadModule = (LoadSubModuleFunc) GetProcAddress(module.ModuleHandle, "LoadSubModule");
		module.UnloadModule = (UnloadSubModuleFunc) GetProcAddress(module.ModuleHandle, "UnloadSubModule");

		// Try to load modules
		if ((module.LoadModule != NULL) && (module.UnloadModule != NULL))
		{
			ModuleLoadParameters loadParams = {0};
			loadParams.Settings = moduleSettings;

			__try
			{
				if (module.LoadModule(&loadParams))
					if ((loadParams.ApiVersion == ACTUAL_API_VERSION) && (loadParams.OpenStorage != NULL)
						&& (loadParams.CloseStorage != NULL) && (loadParams.GetItem != NULL) && (loadParams.ExtractItem != NULL))
					{
						module.ModuleVersion = loadParams.ModuleVersion;
						module.OpenStorage = loadParams.OpenStorage;
						module.CloseStorage = loadParams.CloseStorage;
						module.GetNextItem = loadParams.GetItem;
						module.Extract = loadParams.ExtractItem;

						return true;
					}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				//Nothing to do here. Module unload code is below.
#ifdef _DEBUG
				wchar_t msgText[1024] = {0};
				swprintf_s(msgText, ARRAY_SIZE(msgText), L"Module load exception (Error: %u, Path: %s)", _exception_code(), module.LibraryFile());
				MessageBox(0, msgText, L"Error", MB_OK);
#endif
			}
		}

		FreeLibrary(module.ModuleHandle);
		return false;
	}
#ifdef _DEBUG
	else
	{
		DWORD err = GetLastError();
		
		wchar_t msgText[1024] = {0};
		swprintf_s(msgText, ARRAY_SIZE(msgText), L"Can not load modules (Error: %u, Path: %s)", err, module.LibraryFile());
		MessageBox(0, msgText, L"Error", MB_OK);
	}
#endif
	
	return false;
}
