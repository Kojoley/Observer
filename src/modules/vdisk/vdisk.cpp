// vdisk.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <vcclr.h>
#include <msclr/marshal.h>
#include "ModuleDef.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Reflection;
using namespace DiscUtils;

ref class VDFileInfo
{
public:
	DiscFileInfo^ FileRef;
	LogicalVolumeInfo^ VolumeRef;
	int VolumeIndex;
};

ref class AssemblyLoaderEx
{
public:
	static Assembly^ AssemblyResolveEventHandler( Object^ sender, ResolveEventArgs^ args )
	{
		if (args->Name->StartsWith("DiscUtils"))
		{
			String ^strOwnPath = Assembly::GetAssembly(AssemblyLoaderEx::typeid)->Location;
			String ^strTargetPath = IO::Path::GetDirectoryName(strOwnPath) + "\\DiscUtils.dll";
			
			return Assembly::LoadFile(strTargetPath);
		}
		return nullptr;
	}

};

struct VDisk
{
	gcroot<VirtualDisk^> pVdiskObj;
	gcroot< List<VDFileInfo^>^ > vItems;
};

static void EnumFilesInVolume(VDisk* vdObj, DiscDirectoryInfo^ dirInfo, LogicalVolumeInfo^ vol, int volIndex)
{
	array<DiscDirectoryInfo^> ^subDirList = dirInfo->GetDirectories();
	for (int i = 0; i < subDirList->Length; i++)
	{
		EnumFilesInVolume(vdObj, subDirList[i], vol, volIndex);
	}

	array<DiscFileInfo^> ^fileList = dirInfo->GetFiles();
	for (int i = 0; i < fileList->Length; i++)
	{
		VDFileInfo^ fileInfo = gcnew VDFileInfo();
		fileInfo->FileRef = fileList[i];
		fileInfo->VolumeRef = vol;
		fileInfo->VolumeIndex = volIndex;

		vdObj->vItems->Add(fileInfo);
	}
}

static ::FILETIME DateTimeToFileTime(DateTime dt)
{
	::FILETIME ft = {0};
	return ft;
}

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	String ^strPath = gcnew String(params.FilePath);
	VirtualDisk ^vdisk = VirtualDisk::OpenDisk(strPath, IO::FileAccess::Read);
	if (vdisk != nullptr && vdisk->IsPartitioned)
	{
		VDisk* vdObj = new VDisk();
		vdObj->pVdiskObj = vdisk;
		vdObj->vItems = gcnew List<VDFileInfo^>();
		
		VolumeManager^ volm = gcnew VolumeManager(vdisk);

		array<LogicalVolumeInfo^> ^logvol = volm->GetLogicalVolumes();
		for (int i = 0; i < logvol->Length; i++)
		{
			LogicalVolumeInfo^ vi = logvol[i];
			array<DiscUtils::FileSystemInfo^> ^fsinfo = FileSystemManager::DetectDefaultFileSystems(vi);
			if (fsinfo == nullptr || fsinfo->Length == 0) continue;

			DiscFileSystem^ dfs = fsinfo[0]->Open(vi);
			EnumFilesInVolume(vdObj, dfs->Root, vi, i);
		}

		*storage = vdObj;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Virtual Disk");
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");

		return SOR_SUCCESS;
	}

	vdisk = nullptr;
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	VDisk* vdObj = (VDisk*) storage;
	if (vdObj != NULL)
	{
		vdObj->pVdiskObj = nullptr;
		vdObj->vItems = nullptr;
		delete vdObj;
	}
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	VDisk* vdObj = (VDisk*) storage;
	if (vdObj == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (item_index >= vdObj->vItems->Count)
		return GET_ITEM_NOMOREITEMS;

	List<VDFileInfo^> ^fileList = vdObj->vItems;
	VDFileInfo^ fileInfo = fileList[item_index];
	
	LARGE_INTEGER liSize;
	liSize.QuadPart = fileInfo->FileRef->Length;

	// Helper class for String^ to wchar_t* conversion
	msclr::interop::marshal_context ctx;

	wcscpy_s(item_path, path_size, ctx.marshal_as<const wchar_t*>(fileInfo->FileRef->FullName));

	memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
	wcscpy_s(item_data->cFileName, MAX_PATH, ctx.marshal_as<const wchar_t*>(fileInfo->FileRef->Name));
	item_data->dwFileAttributes = (DWORD) fileInfo->FileRef->Attributes;
	item_data->nFileSizeLow = liSize.LowPart;
	item_data->nFileSizeHigh = liSize.HighPart;
	item_data->ftCreationTime = DateTimeToFileTime(fileInfo->FileRef->CreationTimeUtc);
	item_data->ftLastWriteTime = DateTimeToFileTime(fileInfo->FileRef->LastWriteTimeUtc);
	
	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	return SER_ERROR_SYSTEM;
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

	AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(AssemblyLoaderEx::AssemblyResolveEventHandler);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
