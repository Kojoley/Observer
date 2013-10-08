#ifndef IS6CabFile_h__
#define IS6CabFile_h__

#include "ISCabFile.h"

namespace IS6
{

#include "IS6_CabStructs.h"

struct DataVolume
{
	DWORD VolumeIndex;
	std::wstring FilePath;
	HANDLE FileHandle;
	__int64 FileSize;
	CABHEADER Header;
};

class IS6CabFile : public ISCabFile
{
protected:
	CABHEADER m_Header;
	CABDESC* m_pCabDesc;
	DWORD* m_pDFT;

	std::vector<DWORD> m_vValidFiles;
	std::vector<FILEGROUPDESC> m_vFileGroups;
	std::vector<DWORD> m_vComponents;

	std::vector<DataVolume*> m_vVolumes;

	void GenerateInfoFile();
	virtual bool IsVersionCompatible(DWORD version);
	bool InternalOpen(HANDLE headerFile);

	DataVolume* OpenVolume(DWORD volumeIndex);

public:	
	IS6CabFile();
	~IS6CabFile();

	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	int ExtractFile(int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx);

	void Close();

	DWORD MajorVersion() const;
	bool HasInfoData() const { return true; }
	const wchar_t* GetCompression() const { return L"ZData"; }
};

} // namespace IS5

#endif // IS6CabFile_h__
