#ifndef _CONTENT_STRUCTURES_H_
#define _CONTENT_STRUCTURES_H_

#include "ModuleDef.h"

class ContentTreeNode;
typedef std::map<std::wstring, ContentTreeNode*> SubNodesMap;
typedef std::vector<ContentTreeNode*> ContentNodeList;

class ContentTreeNode
{
private:
	// Support for storages where only files are defined (with path though)
	std::vector<ContentTreeNode* > m_dummyFolders;
	ContentTreeNode* insertDummyDirectory(const wchar_t *name);

	ContentTreeNode* GetSubDir(const wchar_t* name);
	void AddFile(ContentTreeNode* child);
	void Init(int item_index, StorageItemInfo* item_info);

	std::wstring m_strName;
	__int64 m_nSize;
	__int64 m_nPackedSize;
	DWORD m_nAttributes;
	WORD m_nNumberOfHardlinks;
	std::wstring m_strOwner;

public:
	int StorageIndex;
	FILETIME LastModificationTime;
	FILETIME CreationTime;
	
	ContentTreeNode* parent;
	SubNodesMap subdirs;
	SubNodesMap files;

	ContentTreeNode();
	ContentTreeNode(int index, StorageItemInfo* info);
	~ContentTreeNode();

	size_t GetPath(wchar_t* dest, size_t destSize, ContentTreeNode* upRoot = NULL) const;
	std::wstring GetPath(ContentTreeNode* upRoot = NULL) const;
	bool AddChild(wchar_t* path, ContentTreeNode* child);

	size_t GetChildCount() { return subdirs.size() + files.size(); };
	ContentTreeNode* GetChildByName(const wchar_t* name);
	bool IsDir() const { return (m_nAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0; }

	__int64 GetSize() const { return IsDir() ? 0 : m_nSize; }
	__int64 GetPackedSize() const { return IsDir() ? 0 : m_nPackedSize; }
	DWORD GetAttributes() const { return m_nAttributes; }
	WORD GetNumberOfHardLinks() const { return m_nNumberOfHardlinks; }
	const wchar_t* GetOwner() const { return m_strOwner.size() > 0 ? m_strOwner.c_str() : nullptr; }

	const wchar_t* Name() const { return m_strName.c_str(); }
	void SetName(const wchar_t* newName) { m_strName = newName; }

	size_t GetSubDirectoriesNum(bool recursive);
};

#endif