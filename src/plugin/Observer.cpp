// Observer.cpp : Defines the exported functions for the DLL application.
// This module contains functions for ANSI version of FAR (1.75+)

#include "StdAfx.h"
#include <far/plugin.hpp>

#include "CommonFunc.h"
#include "ModulesController.h"
#include "PlugLang.h"
#include "FarStorage.h"
#include "InterfaceCommon.h"

#include "Config.h"
#include "RegistrySettings.h"

extern HMODULE g_hDllHandle;
static PluginStartupInfo FarSInfo;
static FarStandardFunctions FSF;

static wchar_t wszPluginLocation[MAX_PATH];
static ModulesController g_pController;

#define CP_FAR_INTERNAL CP_OEMCP

// Settings
#define MAX_PREFIX_SIZE 32
static int optEnabled = TRUE;
static int optUsePrefix = TRUE;
static int optUseExtensionFilters = TRUE;
static int optVerboseModuleLoad = FALSE;
static char optPrefix[MAX_PREFIX_SIZE] = "observe";

// Extended settings
static char optPanelHeaderPrefix[MAX_PREFIX_SIZE] = "";
static int optExtendedCurDir = FALSE;

//-----------------------------------  Local functions ----------------------------------------

struct InitDialogItem
{
	int Type;
	int X1;
	int Y1;
	int X2;
	int Y2;
	int Focus;
	int Selected;
	unsigned int Flags;
	int DefaultButton;
	char *Data;
};


static void InitDialogItems(const struct InitDialogItem *Init, struct FarDialogItem *Item,	int ItemsNumber)
{
	int I;
	const struct InitDialogItem *PInit=Init;
	struct FarDialogItem *PItem=Item;
	for (I=0; I < ItemsNumber; I++,PItem++,PInit++)
	{
		PItem->Type=PInit->Type;
		PItem->X1=PInit->X1;
		PItem->Y1=PInit->Y1;
		PItem->X2=PInit->X2;
		PItem->Y2=PInit->Y2;
		PItem->Focus=PInit->Focus;
		PItem->Selected=PInit->Selected;
		PItem->Flags=PInit->Flags;
		PItem->DefaultButton=PInit->DefaultButton;
		strcpy_s(PItem->Data, sizeof(PItem->Data) / sizeof(PItem->Data[0]), PInit->Data);
	}
}

static const char* GetLocMsg(int MsgID)
{
	return FarSInfo.GetMsg(FarSInfo.ModuleNumber, MsgID);
}

static void LoadSettings(Config* cfg)
{
	// Load static settings from .ini file.
	ConfigSection* generalCfg = cfg->GetSection(L"General");
	if (generalCfg != NULL)
	{
		generalCfg->GetValue(L"PanelHeaderPrefix", optPanelHeaderPrefix, MAX_PREFIX_SIZE);
		generalCfg->GetValue(L"ExtendedCurDir", optExtendedCurDir);
		generalCfg->GetValue(L"UseExtensionFilters", optUseExtensionFilters);
	}

	// Load dynamic settings from registry (they will overwrite static ones)
	RegistrySettings regOpts(FarSInfo.RootKey);
	if (regOpts.Open())
	{
		regOpts.GetValue(L"Enabled", optEnabled);
		regOpts.GetValue(L"UsePrefix", optUsePrefix);
		regOpts.GetValue("Prefix", optPrefix, MAX_PREFIX_SIZE);

		regOpts.GetValue("PanelHeaderPrefix", optPanelHeaderPrefix, MAX_PREFIX_SIZE);
		regOpts.GetValue(L"ExtendedCurDir", optExtendedCurDir);
		regOpts.GetValue(L"UseExtensionFilters", optUseExtensionFilters);
	}
}

static void SaveSettings()
{
	RegistrySettings regOpts(FarSInfo.RootKey);
	if (regOpts.Open(true))
	{
		regOpts.SetValue(L"Enabled", optEnabled);
		regOpts.SetValue(L"UsePrefix", optUsePrefix);
		regOpts.SetValue("Prefix", optPrefix);
		regOpts.SetValue(L"UseExtensionFilters", optUseExtensionFilters);
	}
}

static string ResolveFullPath(const char* input)
{
	string strVal(input);

	DWORD nBufSize = ExpandEnvironmentStringsA(strVal.c_str(), NULL, 0);
	if (nBufSize > 0)
	{
		char tmpBuf[MAX_PATH] = {0};
		ExpandEnvironmentStringsA(strVal.c_str(), tmpBuf, MAX_PATH);
		strVal = tmpBuf;
	}

	nBufSize = GetFullPathNameA(strVal.c_str(), 0, NULL, NULL);
	if (nBufSize > 0)
	{
		char tmpBuf2[MAX_PATH] = {0};
		GetFullPathNameA(strVal.c_str(), MAX_PATH, tmpBuf2, NULL);
		strVal = tmpBuf2;
	}

	return strVal;
}

static void DisplayMessage(bool isError, bool isInteractive, const char* headerMsg, const char* textMsg, const char* errorItem)
{
	static const char* MsgLines[3];
	MsgLines[0] = headerMsg;
	MsgLines[1] = textMsg;
	MsgLines[2] = errorItem;

	int linesNum = (errorItem) ? 3 : 2;
	int flags = 0;
	if (isError) flags |= FMSG_WARNING;
	if (isInteractive) flags |= FMSG_MB_OK;
	
	FarSInfo.Message(FarSInfo.ModuleNumber, flags, NULL, MsgLines, linesNum, 0);
}

static void DisplayMessage(bool isError, bool isInteractive, int headerMsgID, int textMsgID, const char* errorItem)
{
	DisplayMessage(isError, isInteractive, GetLocMsg(headerMsgID), GetLocMsg(textMsgID), errorItem);
}

static bool StoragePasswordQuery(char* buffer, size_t bufferSize)
{
	return FarSInfo.InputBox(GetLocMsg(MSG_PLUGIN_NAME), GetLocMsg(MSG_OPEN_PASS_REQUIRED), NULL, NULL, buffer, bufferSize-1, NULL, FIB_PASSWORD | FIB_NOUSELASTHISTORY) == TRUE;
}

static int SelectModuleToOpenFileAs()
{
	size_t nNumModules = g_pController.NumModules();

	FarMenuItem* MenuItems = new FarMenuItem[nNumModules];
	memset(MenuItems, 0, nNumModules * sizeof(FarMenuItem));
	for (size_t i = 0; i < nNumModules; i++)
	{
		const ExternalModule& modInfo = g_pController.GetModule(i);
		WideCharToMultiByte(CP_FAR_INTERNAL, 0, modInfo.Name(), -1, MenuItems[i].Text, ARRAY_SIZE(MenuItems[i].Text), NULL, NULL);
	}

	int nMSel = FarSInfo.Menu(FarSInfo.ModuleNumber, -1, -1, 0, 0, GetLocMsg(MSG_OPEN_SELECT_MODULE), NULL, NULL, NULL, NULL, MenuItems, (int) nNumModules);

	delete [] MenuItems;
	return nMSel;
}

void ReportFailedModules(vector<FailedModuleInfo> &failedModules)
{
	if (!optVerboseModuleLoad || (failedModules.size() == 0)) return;

	//TODO: show dialog
}

//-----------------------------------  Content functions ----------------------------------------

static HANDLE OpenStorage(const char* Name, bool applyExtFilters, int moduleIndex)
{
	wchar_t wszWideName[MAX_PATH] = {0};
	MultiByteToWideChar(CP_FAR_INTERNAL, 0, Name, strlen(Name), wszWideName, MAX_PATH);

	StorageObject *storage = new StorageObject(&g_pController, StoragePasswordQuery);
	if (!storage->Open(wszWideName, applyExtFilters, moduleIndex))
	{
		delete storage;
		return INVALID_HANDLE_VALUE;
	}

	char szDialogText[100] = {0};
	char szModuleName[50] = {0};
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, storage->GetModuleName(), -1, szModuleName, ARRAY_SIZE(szModuleName), NULL, NULL);
	sprintf_s(szDialogText, ARRAY_SIZE(szDialogText), GetLocMsg(MSG_OPEN_LIST), szModuleName);

	HANDLE hScreen = FarSInfo.SaveScreen(0, 0, -1, -1);
	DisplayMessage(false, false, GetLocMsg(MSG_PLUGIN_NAME), szDialogText, NULL);

	HANDLE hResult = INVALID_HANDLE_VALUE;
	bool listAborted;
	if (storage->ReadFileList(listAborted))
	{
		hResult = (HANDLE) storage;
	}
	else
	{
		if (!listAborted)
			DisplayMessage(true, true, MSG_OPEN_CONTENT_ERROR, MSG_OPEN_INVALID_ITEM, NULL);
		delete storage;
	}

	FarSInfo.RestoreScreen(hScreen);

	return hResult;
}

static void CloseStorage(HANDLE hStorage)
{
	StorageObject *sobj = (StorageObject*) hStorage;
	sobj->Close();
	delete sobj;
}

static bool GetSelectedPanelFilePath(string& nameStr)
{
	nameStr.clear();

	PanelInfo pi;
	memset(&pi, 0, sizeof(pi));
	if (FarSInfo.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi))
		if ((pi.SelectedItemsNumber == 1) && (pi.PanelType == PTYPE_FILEPANEL)
			&& ((pi.SelectedItems[0].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
		{
			char szPathBuf[MAX_PATH] = {0};

			strcpy_s(szPathBuf, MAX_PATH, pi.CurDir);
			IncludeTrailingPathDelim(szPathBuf, MAX_PATH);
			strcat_s(szPathBuf, MAX_PATH, pi.SelectedItems[0].FindData.cFileName);

			nameStr = szPathBuf;
		}

	return (nameStr.size() > 0);
}

//-----------------------------------  Callback functions ----------------------------------------

static int CALLBACK ExtractProgress(HANDLE context, __int64 ProcessedBytes)
{
	// Check for ESC pressed
	if (CheckEsc())
		return FALSE;

	ProgressContext* pc = (ProgressContext*) context;
	pc->nProcessedFileBytes += ProcessedBytes;

	int nFileProgress = (pc->nCurrentFileSize > 0) ? (int) ((pc->nProcessedFileBytes * 100) / pc->nCurrentFileSize) : 0;
	int nTotalProgress = (pc->nTotalSize > 0) ? (int) (((pc->nTotalProcessedBytes + pc->nProcessedFileBytes) * 100) / pc->nTotalSize) : 0;

	if (nFileProgress != pc->nCurrentProgress)
	{
		pc->nCurrentProgress = nFileProgress;
		
		static char szFileProgressLine[100] = {0};
		sprintf_s(szFileProgressLine, 100, "File: %d/%d. Progress: %2d%% / %2d%%", pc->nCurrentFileNumber, pc->nTotalFiles, nFileProgress, nTotalProgress);

		static const char* InfoLines[4];
		InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
		InfoLines[1] = GetLocMsg(MSG_EXTRACT_EXTRACTING);
		InfoLines[2] = szFileProgressLine;
		InfoLines[3] = pc->szFilePath;

		FarSInfo.Message(FarSInfo.ModuleNumber, 0, NULL, InfoLines, sizeof(InfoLines) / sizeof(InfoLines[0]), 0);
	}

	return TRUE;
}

static int CALLBACK ExtractStart(const ContentTreeNode* item, ProgressContext* context, HANDLE &screen)
{
	screen = FarSInfo.SaveScreen(0, 0, -1, -1);

	context->nCurrentFileNumber++;
	context->nCurrentFileSize = item->GetSize();
	context->nProcessedFileBytes = 0;
	context->nCurrentProgress = -1;

	wchar_t wszSubPath[PATH_BUFFER_SIZE] = {0};
	item->GetPath(wszSubPath, PATH_BUFFER_SIZE);
		
	// Save file name for dialogs
	int nPathLen = wcslen(wszSubPath);
	wchar_t* wszSubPathPtr = wszSubPath;
	if (nPathLen > MAX_PATH) wszSubPathPtr += (nPathLen % MAX_PATH);

	memset(context->szFilePath, 0, MAX_PATH);
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, wszSubPathPtr, wcslen(wszSubPathPtr), context->szFilePath, MAX_PATH, NULL, NULL);
	
	// Shrink file path to fit on console
	CONSOLE_SCREEN_BUFFER_INFO si;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if ((hStdOut != INVALID_HANDLE_VALUE) && GetConsoleScreenBufferInfo(hStdOut, &si))
		FSF.TruncPathStr(context->szFilePath, si.dwSize.X - 16);

	return ExtractProgress((HANDLE)context, 0);
}

static void CALLBACK ExtractDone(ProgressContext* context, HANDLE screen, bool success)
{
	FarSInfo.RestoreScreen(screen);

	if (success)
		context->nTotalProcessedBytes += context->nCurrentFileSize;
}

static int ExtractError(int errorReason, HANDLE context)
{
	ProgressContext* pctx = (ProgressContext*) context;
	
	static const char* InfoLines[7];
	InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	InfoLines[1] = GetLocMsg(MSG_EXTRACT_FAILED);
	InfoLines[2] = pctx->szFilePath;
	InfoLines[3] = "Retry";
	InfoLines[4] = GetLocMsg(MSG_BTN_SKIP);
	InfoLines[5] = GetLocMsg(MSG_BTN_SKIP_ALL);
	InfoLines[6] = GetLocMsg(MSG_BTN_ABORT);

	int nMsg = FarSInfo.Message(FarSInfo.ModuleNumber, FMSG_WARNING, NULL, InfoLines, sizeof(InfoLines) / sizeof(InfoLines[0]), 4);

	switch (nMsg)
	{
		case 0:
			return EEN_RETRY;
		case 1:
			return EEN_SKIP;
		case 2:
			return EEN_SKIPALL;
		case 3:
			return EEN_ABORT;
	}

	return EEN_SKIP;
}
//-----------------------------------  Extract functions ----------------------------------------

// Overwrite values
#define EXTR_OVERWRITE_ASK 1
#define EXTR_OVERWRITE_SILENT 2
#define EXTR_OVERWRITE_SKIP 3
#define EXTR_OVERWRITE_SKIPSILENT 4

static bool AskExtractOverwrite(int &overwrite, WIN32_FIND_DATAW existingFile, const ContentTreeNode* newFile)
{
	__int64 nOldSize = ((__int64) existingFile.nFileSizeHigh >> 32) + existingFile.nFileSizeLow;
	__int64 nNewSize = newFile->GetSize();
	
	SYSTEMTIME stOldUTC, stOldLocal;
	FileTimeToSystemTime(&existingFile.ftLastWriteTime, &stOldUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stOldUTC, &stOldLocal);

	SYSTEMTIME stNewUTC, stNewLocal;
	FileTimeToSystemTime(&newFile->LastModificationTime, &stNewUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stNewUTC, &stNewLocal);

	char szFileName[MAX_PATH] = {0};
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, newFile->Name(), -1, szFileName, MAX_PATH, NULL, NULL);
	
	static char szDialogLine1[120] = {0};
	sprintf_s(szDialogLine1, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), GetLocMsg(MSG_EXTRACT_OVERWRITE), szFileName);
	static char szDialogLine2[120] = {0};
	sprintf_s(szDialogLine2, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), "%21s  %s", "Size", "Last Modification");
	static char szDialogLine3[120] = {0};
	sprintf_s(szDialogLine3, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), "Old file: %11u  %5u-%02u-%02u %02u:%02u",
		(UINT) nOldSize, stOldLocal.wYear, stOldLocal.wMonth, stOldLocal.wDay, stOldLocal.wHour, stOldLocal.wMinute);
	static char szDialogLine4[120] = {0};
	sprintf_s(szDialogLine4, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), "New file: %11u  %5u-%02u-%02u %02u:%02u",
		(UINT) nNewSize, stNewLocal.wYear, stNewLocal.wMonth, stNewLocal.wDay, stNewLocal.wHour, stNewLocal.wMinute);
	
	static const char* DlgLines[10];
	DlgLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	DlgLines[1] = szDialogLine1;
	DlgLines[2] = szDialogLine2;
	DlgLines[3] = szDialogLine3;
	DlgLines[4] = szDialogLine4;
	DlgLines[5] = GetLocMsg(MSG_OVERWRITE);
	DlgLines[6] = GetLocMsg(MSG_OVERWRITE_ALL);
	DlgLines[7] = GetLocMsg(MSG_BTN_SKIP);
	DlgLines[8] = GetLocMsg(MSG_BTN_SKIP_ALL);
	DlgLines[9] = GetLocMsg(MSG_BTN_CANCEL);

	int nMsg = FarSInfo.Message(FarSInfo.ModuleNumber, FMSG_WARNING, NULL, DlgLines, sizeof(DlgLines) / sizeof(DlgLines[0]), 5);
	
	if ((nMsg == 4) // Cancel is pressed
		|| (nMsg == -1)) //Escape is pressed
		return false;
	else
	{
		overwrite = nMsg + 1;
		return true;
	}
}

static int ExtractStorageItem(StorageObject* storage, ContentTreeNode* item, wstring& destPath, bool silent, int &doOverwrite, bool &skipOnError, ProgressContext* pctx)
{
	if (!item || !storage || item->IsDir()) return FALSE;

	// Check for ESC pressed
	if (CheckEsc())	return FALSE;

	// Ask about overwrite if needed
	WIN32_FIND_DATAW fdExistingFile = {0};
	bool fAlreadyExists = FileExists(destPath, &fdExistingFile);

	if (!silent && fAlreadyExists)
	{
		if (doOverwrite == EXTR_OVERWRITE_ASK)
			if (!AskExtractOverwrite(doOverwrite, fdExistingFile, item))
				return FALSE;
		
		// Check either ask result or present value
		if (doOverwrite == EXTR_OVERWRITE_SKIP)
		{
			doOverwrite = EXTR_OVERWRITE_ASK;
			return TRUE;
		}
		else if (doOverwrite == EXTR_OVERWRITE_SKIPSILENT)
		{
			return TRUE;
		}
	}

	// Create directory if needed
	if (!fAlreadyExists)
	{
		wstring strTargetDir = GetDirectoryName(destPath, false);
		if (strTargetDir.length() > 0)
		{
			if (!ForceDirectoryExist(strTargetDir.c_str()))
			{
				if (!silent)
				{
					char tmpPathBuf[MAX_PATH] = {0};
					WideCharToMultiByte(CP_FAR_INTERNAL, 0, strTargetDir.c_str(), -1, tmpPathBuf, MAX_PATH, NULL, NULL);
					DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_DIR_CREATE_ERROR, tmpPathBuf);
				}
				
				return FALSE;
			}
		}
	}

	// Remove read-only attribute from target file if present
	if (fAlreadyExists && (fdExistingFile.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
	{
		SetFileAttributes(destPath.c_str(), fdExistingFile.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
	}

	HANDLE hScreen;

	int ret;
	do
	{
		// Set extract params
		ExtractOperationParams params;
		params.ItemIndex = item->StorageIndex;
		params.Flags = 0;
		params.DestPath = destPath.c_str();
		params.Callbacks.FileProgress = ExtractProgress;
		params.Callbacks.signalContext = pctx;

		ExtractStart(item, pctx, hScreen);
		ret = storage->Extract(params);
		ExtractDone(pctx, hScreen, ret == SER_SUCCESS);

		if ((ret == SER_ERROR_WRITE) || (ret == SER_ERROR_READ))
		{
			int errorResp = EEN_ABORT;
			if (skipOnError)
				errorResp = EEN_SKIP;
			else if (!silent)
				errorResp = ExtractError(ret, pctx);

			switch (errorResp)
			{
				case EEN_ABORT:
					ret = SER_USERABORT;
					break;
				case EEN_SKIP:
					ret = SER_SUCCESS;
					break;
				case EEN_SKIPALL:
					ret = SER_SUCCESS;
					skipOnError = true;
					break;
			}
		}
		else if (ret == SER_ERROR_SYSTEM)
		{
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_FAILED, pctx->szFilePath);
		}

	} while ((ret != SER_SUCCESS) && (ret != SER_ERROR_SYSTEM) && (ret != SER_USERABORT));

	// If extraction is successful set file attributes if present
	if ((ret == SER_SUCCESS) && (item->GetAttributes() != 0))
	{
		SetFileAttributes(destPath.c_str(), item->GetAttributes());
	}

	return (ret == SER_SUCCESS);
}

bool ConfirmExtract(int NumFiles, int NumDirectories, ExtractSelectedParams &params)
{
	static char szDialogLine1[120] = {0};
	sprintf_s(szDialogLine1, ARRAY_SIZE(szDialogLine1), GetLocMsg(MSG_EXTRACT_CONFIRM), NumFiles, NumDirectories);

	InitDialogItem InitItems []={
		/*0*/{DI_DOUBLEBOX, 3, 1, 60, 9, 0, 0, 0,0, (char*) GetLocMsg(MSG_EXTRACT_TITLE)},
		/*1*/{DI_TEXT,	    5, 2,  0, 2, 0, 0, 0, 0, szDialogLine1},
		/*2*/{DI_EDIT,	    5, 3, 58, 3, 1, 0, DIF_EDITEXPAND,0, (char*) params.strDestPath.c_str()},
		/*3*/{DI_TEXT,	    3, 4,  0, 4, 0, 0, DIF_BOXCOLOR|DIF_SEPARATOR, 0, ""},
		/*4*/{DI_CHECKBOX,  5, 5,  0, 5, 0, params.nOverwriteExistingFiles, DIF_3STATE, 0, (char*) GetLocMsg(MSG_EXTRACT_DEFOVERWRITE)},
		/*5*/{DI_CHECKBOX,  5, 6,  0, 6, 0, params.nPathProcessing, DIF_3STATE, 0, (char*) GetLocMsg(MSG_EXTRACT_KEEPPATHS)},
		/*6*/{DI_TEXT,	    3, 7,  0, 7, 0, 0, DIF_BOXCOLOR|DIF_SEPARATOR, 0, ""},
		/*7*/{DI_BUTTON,	0, 8,  0, 8, 0, 0, DIF_CENTERGROUP, 1, (char*) GetLocMsg(MSG_BTN_EXTRACT)},
		/*8*/{DI_BUTTON,    0, 8,  0, 8, 0, 0, DIF_CENTERGROUP, 0, (char*) GetLocMsg(MSG_BTN_CANCEL)},
	};
	FarDialogItem DialogItems[sizeof(InitItems)/sizeof(InitItems[0])];

	InitDialogItems(InitItems, DialogItems, ARRAY_SIZE(InitItems));

	int ExitCode = FarSInfo.Dialog(FarSInfo.ModuleNumber, -1, -1, 64, 11, "ObserverExtract", DialogItems, ARRAY_SIZE(DialogItems));

	if (ExitCode == 7)
	{
		params.nOverwriteExistingFiles = DialogItems[4].Selected;
		params.nPathProcessing = DialogItems[5].Selected;
		params.strDestPath = ResolveFullPath(DialogItems[2].Data);
		return true;
	}

	return false;
}

static bool ItemSortPred(ContentTreeNode* item1, ContentTreeNode* item2)
{
	return (item1->StorageIndex < item2->StorageIndex);
}

//-----------------------------------  Export functions ----------------------------------------

int WINAPI GetMinFarVersion(void)
{
	return FARMANAGERVERSION;
}

void WINAPI SetStartupInfo(const struct PluginStartupInfo *Info)
{
	FarSInfo = *Info;
	FSF = *Info->FSF;
	FarSInfo.FSF = &FSF;

	if (GetModuleFileNameW(g_hDllHandle, wszPluginLocation, MAX_PATH))
	{
		CutFileNameFromPath(wszPluginLocation, true);
	}
	else
	{
		wmemset(wszPluginLocation, 0, MAX_PATH);
	}

	wstring strConfigLocation(wszPluginLocation);

	Config cfg;
	cfg.ParseFile(strConfigLocation + CONFIG_FILE);
	cfg.ParseFile(strConfigLocation + CONFIG_USER_FILE);

	LoadSettings(&cfg);

	vector<FailedModuleInfo> fails;
	g_pController.Init(wszPluginLocation, &cfg, fails);
	ReportFailedModules(fails);
}

void WINAPI GetPluginInfo(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(PluginInfo);
	Info->Flags = 0;

	static const char *PluginMenuStrings[1];
	PluginMenuStrings[0] = GetLocMsg(MSG_PLUGIN_NAME);
	
	static const char *ConfigMenuStrings[1];
	ConfigMenuStrings[0] = GetLocMsg(MSG_PLUGIN_CONFIG_NAME);

	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = sizeof(PluginMenuStrings) / sizeof(PluginMenuStrings[0]);
	Info->PluginConfigStrings = ConfigMenuStrings;
	Info->PluginConfigStringsNumber = sizeof(ConfigMenuStrings) / sizeof(ConfigMenuStrings[0]);
	Info->CommandPrefix = optPrefix;
}

void WINAPI ExitFAR(void)
{
	g_pController.Cleanup();
}

int WINAPI Configure(int ItemNumber)
{
	InitDialogItem InitItems []={
	/*00*/ DI_DOUBLEBOX, 3,1, 40, 8, 0, 0, 0,0, (char*) GetLocMsg(MSG_CONFIG_TITLE),
	/*01*/ DI_CHECKBOX,  5,2,  0, 2, 0, 0, 0,0, (char*) GetLocMsg(MSG_CONFIG_ENABLE),
	/*02*/ DI_CHECKBOX,  5,3,  0, 2, 0, 0, 0,0, (char*) GetLocMsg(MSG_CONFIG_PREFIX),
	/*03*/ DI_FIXEDIT,	 7,4, 17, 3, 0, 0, 0,0, "",
	/*04*/ DI_CHECKBOX,  5,5,  0, 5, 0, 0, 0,0, (char*) GetLocMsg(MSG_CONFIG_USEEXTFILTERS),
	/*05*/ DI_TEXT,		 5,6,  0, 6, 0, 0, DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
	/*06*/ DI_BUTTON,	 0,7,  0, 7, 1, 0, DIF_CENTERGROUP,1,"OK",
	/*07*/ DI_BUTTON,    0,7,  0, 7, 0, 0, DIF_CENTERGROUP,0,(char*) GetLocMsg(MSG_BTN_CANCEL)
	};
	FarDialogItem DialogItems[sizeof(InitItems)/sizeof(InitItems[0])];

	InitDialogItems(InitItems,DialogItems, sizeof(InitItems)/sizeof(InitItems[0]));

	DialogItems[1].Selected = optEnabled;
	DialogItems[2].Selected = optUsePrefix;
	strcpy_s(DialogItems[3].Data, sizeof(DialogItems[3].Data) / sizeof(DialogItems[3].Data[0]), optPrefix);
	DialogItems[4].Selected = optUseExtensionFilters;

	int ExitCode = FarSInfo.Dialog(FarSInfo.ModuleNumber, -1, -1, 44, 10, "ObserverConfig", DialogItems, sizeof(DialogItems)/sizeof(DialogItems[0]));

	if ((ExitCode == 6) && (strlen(DialogItems[3].Data) < MAX_PREFIX_SIZE))
	{
		optEnabled = DialogItems[1].Selected != 0;
		optUsePrefix = DialogItems[2].Selected != 0;
		strcpy_s(optPrefix, MAX_PREFIX_SIZE, DialogItems[3].Data);
		optUseExtensionFilters = DialogItems[4].Selected != 0;

		SaveSettings();
	
		return TRUE;
	}

	return FALSE;
}

void WINAPI ClosePlugin(HANDLE hPlugin)
{
	if (hPlugin != NULL)
		CloseStorage(hPlugin);
}

HANDLE WINAPI OpenPlugin(int OpenFrom, INT_PTR Item)
{
	// Unload plug-in if no submodules loaded
	if (g_pController.NumModules() == 0)
		return 0;
	
	string strFullSourcePath;
	string strSubPath;
	int nOpenModuleIndex = -1;
	
	if ((OpenFrom == OPEN_COMMANDLINE) && optUsePrefix)
	{
		char* szLocalNameBuffer = _strdup((char *) Item);
		FSF.Unquote(szLocalNameBuffer);

		// Find starting subdirectory if specified
		char* szColonPos = strrchr(szLocalNameBuffer, ':');
		if (szColonPos != NULL && (szColonPos - szLocalNameBuffer) > 2)
		{
			*szColonPos = 0;
			strSubPath = szColonPos + 1;
		}

		strFullSourcePath = ResolveFullPath(szLocalNameBuffer);

		free(szLocalNameBuffer);
	}
	else if (OpenFrom == OPEN_PLUGINSMENU)
	{
		if (GetSelectedPanelFilePath(strFullSourcePath))
		{
			FarMenuItem MenuItems[2] = {0};
			strcpy_s(MenuItems[0].Text, ARRAY_SIZE(MenuItems[0].Text), GetLocMsg(MSG_OPEN_FILE));
			strcpy_s(MenuItems[1].Text, ARRAY_SIZE(MenuItems[1].Text), GetLocMsg(MSG_OPEN_FILE_AS));

			int nMItem = FarSInfo.Menu(FarSInfo.ModuleNumber, -1, -1, 0, 0, GetLocMsg(MSG_PLUGIN_NAME), NULL, NULL, NULL, NULL, MenuItems, ARRAY_SIZE(MenuItems));

			if (nMItem == -1)
				return INVALID_HANDLE_VALUE;
			else if (nMItem == 1) // Show modules selection dialog
			{
				int nSelectedModItem = SelectModuleToOpenFileAs();
				if (nSelectedModItem >= 0)
					nOpenModuleIndex = nSelectedModItem;
				else
					return INVALID_HANDLE_VALUE;
			}
		}
	}

	HANDLE hOpenResult = (strFullSourcePath.size() > 0) ? OpenStorage(strFullSourcePath.c_str(), false, nOpenModuleIndex) : INVALID_HANDLE_VALUE;

	if ( (hOpenResult != INVALID_HANDLE_VALUE) && (strSubPath.size() > 0) )
		SetDirectory(hOpenResult, strSubPath.c_str(), 0);

	return hOpenResult;
}

HANDLE WINAPI OpenFilePlugin(char *Name, const unsigned char *Data, int DataSize)
{
	if (!Name || !optEnabled)
		return INVALID_HANDLE_VALUE;

	HANDLE hOpenResult = OpenStorage(Name, optUseExtensionFilters != 0, -1);
	return hOpenResult;
}

int WINAPI GetFindData(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	StorageObject* info = (StorageObject *) hPlugin;
	if (!info || !info->CurrentDir()) return FALSE;

	int nTotalItems = info->CurrentDir()->GetChildCount();
	*pItemsNumber = nTotalItems;

	// Zero items - exit now
	if (nTotalItems == 0) return TRUE;

	*pPanelItem = (PluginPanelItem *) malloc(nTotalItems * sizeof(PluginPanelItem));
	PluginPanelItem *panelItem = *pPanelItem;

	// Display all directories
	for (SubNodesMap::const_iterator cit = info->CurrentDir()->subdirs.begin(); cit != info->CurrentDir()->subdirs.end(); cit++)
	{
		memset(panelItem, 0, sizeof(PluginPanelItem));

		const ContentTreeNode* node = (cit->second);

		WideCharToMultiByte(CP_FAR_INTERNAL, 0, node->Name(), -1, panelItem->FindData.cFileName, MAX_PATH, NULL, NULL);
		panelItem->FindData.dwFileAttributes = node->GetAttributes();
		panelItem->FindData.ftCreationTime = node->CreationTime;
		panelItem->FindData.ftLastWriteTime = node->LastModificationTime;
		panelItem->FindData.nFileSizeHigh = (node->GetSize() >> 32);
		panelItem->FindData.nFileSizeLow = (DWORD) node->GetSize();

		panelItem++;
	}

	// Display all files
	for (SubNodesMap::const_iterator cit = info->CurrentDir()->files.begin(); cit != info->CurrentDir()->files.end(); cit++)
	{
		memset(panelItem, 0, sizeof(PluginPanelItem));

		const ContentTreeNode* node = (cit->second);

		WideCharToMultiByte(CP_FAR_INTERNAL, 0, node->Name(), -1, panelItem->FindData.cFileName, MAX_PATH, NULL, NULL);
		panelItem->FindData.dwFileAttributes = node->GetAttributes();
		panelItem->FindData.ftCreationTime = node->CreationTime;
		panelItem->FindData.ftLastWriteTime = node->LastModificationTime;
		panelItem->FindData.nFileSizeHigh = (node->GetSize() >> 32);
		panelItem->FindData.nFileSizeLow = (DWORD) node->GetSize();

		panelItem++;
	}

	return TRUE;
}

void WINAPI FreeFindData(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	free(PanelItem);
}

int WINAPI SetDirectory(HANDLE hPlugin, const char *Dir, int OpMode)
{
	if (hPlugin == NULL || hPlugin == INVALID_HANDLE_VALUE)
		return FALSE;

	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return FALSE;

	if (!Dir || !*Dir) return TRUE;

	const char* szNewDirPtr = Dir;

	// We are in root directory and wanna go upwards (for extended curdir mode)
	if (optExtendedCurDir)
	{
		if (strcmp(Dir, "..") == 0 && info->CurrentDir()->parent == NULL)
		{
			char szTargetPath[MAX_PATH] = {0};
			WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->StoragePath(), -1, szTargetPath, MAX_PATH, NULL, NULL);
			char* lastSlash = strrchr(szTargetPath, '\\');
			if (lastSlash) *lastSlash = 0;

			FarSInfo.Control(hPlugin, FCTL_CLOSEPLUGIN, szTargetPath);
			FarSInfo.Control(INVALID_HANDLE_VALUE, FCTL_SETPANELDIR, szTargetPath);

			// Find position of our container on panel and position cursor there
			PanelInfo pi = {0};
			if (FarSInfo.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi))
			{
				PanelRedrawInfo pri = {0};
				for (int i = 0; i < pi.ItemsNumber; i++)
				{
					if (strcmp(lastSlash+1, pi.PanelItems[i].FindData.cFileName) == 0)
					{
						pri.CurrentItem = i;
						FarSInfo.Control(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, &pri);
						break;
					}

				} // for
			}

			return true;
		}
		else
		{
			const char* szInsideDirPart = strrchr(Dir, ':');
			if (szInsideDirPart && (szInsideDirPart - Dir) > 2)
				szNewDirPtr = szInsideDirPart + 1;
		}
	}

	wchar_t wzDirName[MAX_PATH] = {0};
	MultiByteToWideChar(CP_FAR_INTERNAL, 0, szNewDirPtr, -1, wzDirName, MAX_PATH);
	return info->ChangeCurrentDir(wzDirName);
}

void WINAPI GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *Info)
{
	Info->StructSize = sizeof(OpenPluginInfo);
	
	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return;
	
	static char szCurrentDir[MAX_PATH];
	static char szTitle[512];
	static wchar_t wszCurrentDirPath[PATH_BUFFER_SIZE];
	static char szHostFile[MAX_PATH];
	static KeyBarTitles KeyBar;

	memset(szCurrentDir, 0, sizeof(szCurrentDir));
	memset(szTitle, 0, sizeof(szTitle));
	memset(wszCurrentDirPath, 0, sizeof(wszCurrentDirPath));
	memset(szHostFile, 0, sizeof(szHostFile));

	size_t nDirPrefixLen = 0;
	if (optExtendedCurDir && optUsePrefix && optPrefix[0])
	{
		sprintf_s(szCurrentDir, ARRAY_SIZE(szCurrentDir), "%s:", optPrefix);
		nDirPrefixLen = strlen(szCurrentDir);
		WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->StoragePath(), -1, szCurrentDir + nDirPrefixLen, ARRAY_SIZE(szCurrentDir) - nDirPrefixLen, NULL, NULL);
		strcat_s(szCurrentDir, ARRAY_SIZE(szCurrentDir), ":\\");
		nDirPrefixLen = strlen(szCurrentDir);
	}
		
	info->CurrentDir()->GetPath(wszCurrentDirPath, PATH_BUFFER_SIZE);
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, wszCurrentDirPath, -1, szCurrentDir + nDirPrefixLen, MAX_PATH - nDirPrefixLen, NULL, NULL);

	char szModuleName[32] = {0};
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->GetModuleName(), -1, szModuleName, ARRAY_SIZE(szModuleName), NULL, NULL);
	sprintf_s(szTitle, ARRAY_SIZE(szTitle), "%s%s:\\%s", optPanelHeaderPrefix, szModuleName, szCurrentDir);

	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->StoragePath(), -1, szHostFile, MAX_PATH, NULL, NULL);

	Info->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS;
	Info->CurDir = szCurrentDir;
	Info->PanelTitle = szTitle;
	Info->HostFile = szHostFile;

	// Fill info lines
	static InfoPanelLine pInfoLinesData[IL_LAST];
	size_t nInfoTextSize = sizeof(pInfoLinesData[0].Text) / sizeof(pInfoLinesData[0].Text[0]);
	size_t nInfoDataSize = sizeof(pInfoLinesData[0].Data) / sizeof(pInfoLinesData[0].Data[0]);

	char *szHostFileName = strrchr(szHostFile, '\\');
	if (szHostFileName) szHostFileName++; else szHostFileName = szHostFile;
	
	memset(pInfoLinesData, 0, sizeof(pInfoLinesData));
	strncpy_s(pInfoLinesData[0].Text, nInfoTextSize, szHostFileName, nInfoTextSize-1);
	pInfoLinesData[0].Separator = 1;
	
	strcpy_s(pInfoLinesData[IL_FORMAT].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_FORMAT));
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->GeneralInfo.Format, wcslen(info->GeneralInfo.Format), pInfoLinesData[IL_FORMAT].Data, nInfoDataSize, NULL, NULL);

	strcpy_s(pInfoLinesData[IL_FILES].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_FILES));
	_ultoa_s(info->NumFiles(), pInfoLinesData[IL_FILES].Data, nInfoDataSize, 10);

	strcpy_s(pInfoLinesData[IL_DIRECTORIES].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_DIRS));
	_ultoa_s(info->NumDirectories(), pInfoLinesData[IL_DIRECTORIES].Data, nInfoDataSize, 10);

	strcpy_s(pInfoLinesData[IL_SIZE].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_SIZE));
	_i64toa_s(info->TotalSize(), pInfoLinesData[IL_SIZE].Data, nInfoDataSize, 10);
	InsertCommas(pInfoLinesData[IL_SIZE].Data);

	strcpy_s(pInfoLinesData[IL_PACKED].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_PACKEDSIZE));
	_i64toa_s(info->TotalPackedSize(), pInfoLinesData[IL_PACKED].Data, nInfoDataSize, 10);
	InsertCommas(pInfoLinesData[IL_PACKED].Data);

	strcpy_s(pInfoLinesData[IL_COMPRESS].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_COMPRESSION));
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->GeneralInfo.Compression, wcslen(info->GeneralInfo.Compression), pInfoLinesData[IL_COMPRESS].Data, nInfoDataSize, NULL, NULL);
	pInfoLinesData[IL_COMPRESS].Data[STORAGE_PARAM_MAX_LEN] = 0;

	strcpy_s(pInfoLinesData[IL_COMMENT].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_COMMENT));
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->GeneralInfo.Comment, wcslen(info->GeneralInfo.Comment), pInfoLinesData[IL_COMMENT].Data, nInfoDataSize, NULL, NULL);
	pInfoLinesData[IL_COMMENT].Data[STORAGE_PARAM_MAX_LEN] = 0;

	strcpy_s(pInfoLinesData[IL_CREATED].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_CREATED));
	SYSTEMTIME st;
	if (info->GeneralInfo.Created.dwHighDateTime && FileTimeToSystemTime(&info->GeneralInfo.Created, &st))
		sprintf_s(pInfoLinesData[IL_CREATED].Data, nInfoDataSize, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
	else
		strcpy_s(pInfoLinesData[IL_CREATED].Data, nInfoDataSize, "-");
	
	Info->InfoLinesNumber = sizeof(pInfoLinesData) / sizeof(pInfoLinesData[0]);
	Info->InfoLines = pInfoLinesData;

	memset(&KeyBar, 0, sizeof(KeyBar));
	KeyBar.ShiftTitles[0] = "";
	KeyBar.AltTitles[6-1] = (char*) GetLocMsg(MSG_ALTF6);
	Info->KeyBar = &KeyBar;
}

int WINAPI GetFiles(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, char *DestPath, int OpMode)
{
	if (Move || !DestPath || (OpMode & OPM_FIND) || !ItemsNumber)
		return 0;

	// Check for single '..' item, do not show confirm dialog
	if ((ItemsNumber == 1) && (strcmp(PanelItem[0].FindData.cFileName, "..") == 0))
		return 0;

	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return 0;

	ContentNodeList vcExtractItems;
	__int64 nTotalExtractSize = 0;
	wchar_t wszItemNameBuf[MAX_PATH] = {0};
	int nExtNumFiles = 0, nExtNumDirs = 0;

	// Collect all indices of items to extract
	for (int i = 0; i < ItemsNumber; i++)
	{
		PluginPanelItem pItem = PanelItem[i];
		if (strcmp(pItem.FindData.cFileName, "..") == 0) continue;

		wmemset(wszItemNameBuf, 0, MAX_PATH);
		MultiByteToWideChar(CP_FAR_INTERNAL, 0, pItem.FindData.cFileName, strlen(pItem.FindData.cFileName), wszItemNameBuf, MAX_PATH);

		ContentTreeNode* child = info->CurrentDir()->GetChildByName(wszItemNameBuf);
		if (child) CollectFileList(child, vcExtractItems, nTotalExtractSize, true);

		if (pItem.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			nExtNumDirs++;
		else
			nExtNumFiles++;
	} //for

	// Check if we have something to extract
	if (vcExtractItems.size() == 0)
		return 0;

	// Confirm extraction
	ExtractSelectedParams extrParams;
	extrParams.strDestPath = DestPath;
	extrParams.bSilent = (OpMode & OPM_SILENT) != 0;
	extrParams.bShowProgress = (OpMode & OPM_FIND) == 0;

	if (!extrParams.bSilent)
	{
		IncludeTrailingPathDelim(extrParams.strDestPath);
		if (!ConfirmExtract(nExtNumFiles, nExtNumDirs, extrParams))
			return -1;
	}

	// Items should be sorted (e.g. for access to solid archives)
	sort(vcExtractItems.begin(), vcExtractItems.end(), ItemSortPred);

	// Prepare destination path
	wchar_t wszWideDestPath[MAX_PATH] = {0};
	MultiByteToWideChar(CP_FAR_INTERNAL, 0, extrParams.strDestPath.c_str(), -1, wszWideDestPath, MAX_PATH);
	IncludeTrailingPathDelim(wszWideDestPath, MAX_PATH);
	
	if (!ForceDirectoryExist(wszWideDestPath))
	{
		if (!extrParams.bSilent)
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_DIR_CREATE_ERROR, NULL);
		return 0;
	}

	if (!IsEnoughSpaceInPath(wszWideDestPath, nTotalExtractSize))
	{
		if (!extrParams.bSilent)
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_NODISKSPACE, NULL);
		return 0;
	}

	int nExtractResult = TRUE;
	bool skipOnError = false;

	int doOverwrite = EXTR_OVERWRITE_ASK;
	switch (extrParams.nOverwriteExistingFiles)
	{
	case 0:
		doOverwrite = EXTR_OVERWRITE_SKIPSILENT;
		break;
	case 1:
		doOverwrite = EXTR_OVERWRITE_SILENT;
		break;
	}

	ProgressContext pctx;
	pctx.nTotalFiles = vcExtractItems.size();
	pctx.nTotalSize = nTotalExtractSize;
	pctx.bDisplayOnScreen = extrParams.bShowProgress;

	// Extract all files one by one
	for (ContentNodeList::const_iterator cit = vcExtractItems.begin(); cit != vcExtractItems.end(); cit++)
	{
		ContentTreeNode* nextItem = *cit;

		wstring strFullPath = GetFinalExtractionPath(info, nextItem, wszWideDestPath, extrParams.nPathProcessing);
		if (nextItem->IsDir())
		{
			if (!ForceDirectoryExist(strFullPath.c_str()))
				return 0;
		}
		else
		{
			nExtractResult = ExtractStorageItem(info, nextItem, strFullPath, extrParams.bSilent, doOverwrite, skipOnError, &pctx);
		}

		if (!nExtractResult) break;
	}

	return nExtractResult;
}

int WINAPI ProcessKey(HANDLE hPlugin, int Key, unsigned int ControlState)
{
	if (Key == VK_F6 && ControlState == PKF_ALT)
	{
		StorageObject* info = (StorageObject *) hPlugin;
		if (!info) return FALSE;
		
		PanelInfo pi = {0};
		if (!FarSInfo.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi)) return FALSE;

		size_t nPathSize = wcslen(info->StoragePath());
		char* szTargetDir = (char *) malloc(nPathSize + 1);
		memset(szTargetDir, 0, nPathSize + 1);
		WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->StoragePath(), nPathSize, szTargetDir, nPathSize + 1, NULL, NULL);
		
		char* szLastSlash = strrchr(szTargetDir, '\\');
		if (szLastSlash) *(szLastSlash + 1) = 0;

		GetFiles(hPlugin, pi.SelectedItems, pi.SelectedItemsNumber, 0, szTargetDir, OPM_SILENT);

		free(szTargetDir);
		
		return TRUE;
	}

	return FALSE;
}
