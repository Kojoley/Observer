#ifndef _CONTENT_STRUCTS_H_
#define _CONTENT_STRUCTS_H_

#include "PackageStruct.h"

#define MAX_SHORT_NAME_LEN 14
const FILETIME ZERO_FILE_TIME = {0};

class BasicNode
{
public:
	wchar_t* Key;
	BasicNode* Parent;
	DWORD Attributes;

	wchar_t* SourceName;
	wchar_t* SourceShortName;
	wchar_t* TargetName;
	wchar_t* TargetShortName;

	FILETIME ftCreationTime;
	FILETIME ftModificationTime;

public:
	BasicNode();
	~BasicNode();

	virtual DWORD GetSytemAttributes() = 0;
	virtual wstring GetSourcePath() = 0;
	virtual wstring GetTargetPath() = 0;
	virtual __int64 GetSize() = 0;

	bool IsDir() { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) > 0; };
};

class FileNode : public BasicNode
{
public:
	INT32 FileSize;
	int SequenceMark;
	
	bool IsFake;
	char* FakeFileContent;

public:
	FileNode();
	~FileNode();

	bool Init(FileEntry *entry);

	DWORD GetSytemAttributes();
	wstring GetSourcePath();
	wstring GetTargetPath();
	__int64 GetSize();
};

class DirectoryNode : public BasicNode
{
private:
	DirectoryNode(const DirectoryNode &node) {};

public:
	wchar_t* ParentKey;
	bool IsSpecial;

	vector<DirectoryNode*> SubDirs;
	vector<FileNode*> Files;

	DirectoryNode();
	~DirectoryNode();

	bool Init(DirectoryEntry *entry, bool substDotTargetWithSource);
	bool Init(const wchar_t *dirName);
	void AddSubdir(DirectoryNode *child);
	void AddFile(FileNode *file) { Files.push_back(file); file->Parent = this; }

	int GetFilesCount();
	int GetSubDirsCount();
	__int64 GetTotalSize();

	DWORD GetSytemAttributes();
	wstring GetSourcePath();
	wstring GetTargetPath();
	__int64 GetSize();
};

#endif //_CONTENT_STRUCTS_H_
