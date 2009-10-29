#ifndef _ModulesController_h__
#define _ModulesController_h__

#include "ModuleDef.h"

struct ExternalModule
{
	wchar_t ModuleName[20];
	wchar_t LibraryFile[20];
	HMODULE ModuleHandle;

	LoadSubModuleFunc LoadModule;
	OpenStorageFunc OpenStorage;
	CloseStorageFunc CloseStorage;
	GetItemFunc GetNextItem;
	ExtractFunc Extract;
};

class ModulesController
{
private:
	bool LoadModule(const wchar_t* basePath, ExternalModule &module, const wchar_t* moduleSettings);

public:
	vector<ExternalModule> modules;
	
	ModulesController(void) {};
	~ModulesController(void) { this->Cleanup(); };

	int Init(wchar_t* basePath);
	void Cleanup();
	size_t NumModules() { return modules.size(); };

	int OpenStorageFile(const wchar_t* path, int *moduleIndex, INT_PTR **storage, StorageGeneralInfo *info);
	void CloseStorageFile(int moduleIndex, INT_PTR *storage);
};

#define CONFIG_FILE L"observer.ini"

#endif // _ModulesController_h__