#pragma once

#include "ContentStructs.h"
#include "CabControl.h"
#include "ModuleDef.h"

typedef map<wstring, DirectoryNode*> DirectoryNodesMap;
typedef map<wstring, ComponentEntry> ComponentEntryMap;
typedef map<wstring, wstring> WStringMap;

#define MSI_OPENFLAG_KEEPUNIQUEDIRS 1
#define MSI_OPENFLAG_SHOWSPECIALS 2

#define MSI_COMPRESSION_NONE 0
#define MSI_COMPRESSION_CAB 1
#define MSI_COMPRESSION_MIXED 2

class CMsiViewer
{
private:
	MSIHANDLE m_hMsi;
	DirectoryNode* m_pRootDir;
	vector <MediaEntry> m_vMedias;
	wstring m_strStorageLocation;
	vector <BasicNode*> m_vFlatIndex;
	int m_nSummaryWordCount;
	FILETIME m_ftCreateTime;
	
	CCabControl* m_pCabControl;
	wstring m_strStreamCacheLocation;
	map<wstring, wstring> m_mStreamCache;

	int readDirectories(DirectoryNodesMap &nodemap);
	int readComponents(DirectoryNodesMap &nodemap, ComponentEntryMap &componentmap);
	int readFiles(DirectoryNodesMap &nodemap, ComponentEntryMap &componentmap);
	int readMediaSources();
	int readAppSearch(WStringMap &entries);
	int readCreateFolder(WStringMap &entries);

	int assignParentDirs(DirectoryNodesMap &nodemap, bool processSpecialDirs);
	void removeEmptyFolders(DirectoryNode *root, WStringMap &forcedFolders);
	void mergeDotFolders(DirectoryNode *root);
	void avoidSameFolderNames(DirectoryNode *root);
	void avoidSameFileNames(DirectoryNode *root);
	void checkShortNames(DirectoryNode *root);
	void mergeSameNamedFolders(DirectoryNode *root);

	int generateInfoText();
	int generateLicenseText();
	int dumpRegistryKeys(wstringstream &sstr);
	int dumpFeatures(wstringstream &sstr);
	int dumpShortcuts(wstringstream &sstr);
	int dumpProperties(wstringstream &sstr);

	wstring getStoragePath();
	int cacheInternalStream(const wchar_t* streamName);

	void buildFlatIndex(DirectoryNode* root);

	const wchar_t* getFileStorageName(FileNode* file);
	bool readRealFileAttributes(FileNode* file);

public:
	CMsiViewer(void);
	~CMsiViewer(void);

	int Open(const wchar_t* path, DWORD openFlags);

	int GetTotalDirectories() { return m_pRootDir->GetSubDirsCount(); }
	int GetTotalFiles() { return m_pRootDir->GetFilesCount(); }
	__int64 GetTotalSize() { return m_pRootDir->GetTotalSize(); }
	int GetCompressionType();
	FILETIME GetCreateDateTime() { return m_ftCreateTime; };

	DirectoryNode* GetDirectory(const wchar_t* path);
	FileNode* GetFile(const int fileIndex);

	int DumpFileContent(FileNode *file, const wchar_t *destPath, ExtractProcessCallbacks callbacks);

	bool FindNodeDataByIndex(int itemIndex, LPWIN32_FIND_DATAW dataBuf, wchar_t* itemPathBuf, size_t itemPathBufSize);
};
