#ifndef SfFile_h__
#define SfFile_h__

#include "Streams.h"

struct SFFileEntry
{
	wchar_t LocalPath[MAX_PATH];
	int64_t UnpackedSize;
	int64_t PackedSize;
	bool IsCompressed;
	int64_t DataOffset;
	uint32_t CRC;
};

#define SCRIPT_FILE "irsetup.dat"

class SetupFactoryFile
{
protected:
	CFileStream* m_pInFile;
	std::vector<SFFileEntry> m_vFiles;
	UINT m_nFilenameCodepage;
	int m_nVersion;
	CMemoryStream* m_pScriptData;

	void Init();
	
	bool ReadString(AStream* stream, char* buf);
	bool SkipString(AStream* stream);

public:
	virtual ~SetupFactoryFile() {}

	virtual bool Open(CFileStream* inFile) = 0;
	virtual void Close() = 0;
	virtual int EnumFiles() = 0;

	size_t GetCount() { return m_vFiles.size(); };
	SFFileEntry GetFile(int index) { return m_vFiles[index]; }

	virtual bool ExtractFile(int index, AStream* outStream) = 0;

	void SetFileNameEncoding(UINT codePage) { m_nFilenameCodepage = codePage; }
	UINT GetFileNameEncoding() { return m_nFilenameCodepage; }
	int GetVersion() { return m_nVersion; }
};

SetupFactoryFile* OpenInstaller(const wchar_t* filePath);
void FreeInstaller(SetupFactoryFile* file);

#endif // SfFile_h__
