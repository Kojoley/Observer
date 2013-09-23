#ifndef IS3CabFile_h__
#define IS3CabFile_h__

#include "ISCabFile.h"

namespace IS3
{

#include "IS3_Structs.h"

struct DirEntry
{
	WORD NumFiles;
	char Name[MAX_PATH];
};

struct FileEntry
{
	//
};

class IS3CabFile : public ISCabFile
{
protected:
	CABHEADER m_Header;
	std::vector<DirEntry> m_vDirs;
	std::vector<FileEntry> m_vFiles;
	
	void GenerateInfoFile() { /* Not used for this class */ }
	bool InternalOpen(HANDLE headerFile);

public:
	IS3CabFile();
	~IS3CabFile();

	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	int ExtractFile(int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx);

	void Close();

	DWORD MajorVersion() const { return 3; }
	bool HasInfoData() const { return false; }
};

} // namespace IS3

#endif // IS3CabFile_h__
