#include "StdAfx.h"
#include "CommonFunc.h"

bool CheckEsc()
{
	DWORD dwNumEvents;
	_INPUT_RECORD inRec;

	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
	if (GetNumberOfConsoleInputEvents(hConsole, &dwNumEvents))
		while (PeekConsoleInputA(hConsole, &inRec, 1, &dwNumEvents) && (dwNumEvents > 0))
		{
			ReadConsoleInputA(hConsole, &inRec, 1, &dwNumEvents);
			if ((inRec.EventType == KEY_EVENT) && (inRec.Event.KeyEvent.bKeyDown) && (inRec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))
				return true;
		} //while

		return false;
}

bool FileExists(const wchar_t* path, LPWIN32_FIND_DATAW file_data)
{
	WIN32_FIND_DATAW fdata;

	HANDLE sr = FindFirstFileW(path, &fdata);
	if (sr != INVALID_HANDLE_VALUE)
	{
		if (file_data)
			*file_data = fdata;

		FindClose(sr);
		return true;
	}

	return false;
}

bool DirectoryExists(const wchar_t* path)
{
	WIN32_FIND_DATAW fdata;
	bool fResult = false;

	HANDLE sr = FindFirstFileW(path, &fdata);
	if (sr != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0)
			{
				fResult = true;
				break;
			}
		} while (FindNextFileW(sr, &fdata));

		FindClose(sr);
	}

	return fResult;
}

// Returns total number of items added
int CollectFileList(ContentTreeNode* node, vector<int> &targetlist, __int64 &totalSize, bool recursive)
{
	int numItems = 0;

	if (node->IsDir())
	{
		if (recursive)
		{
			// Iterate through sub directories
			for (SubNodesMap::const_iterator cit = node->subdirs.begin(); cit != node->subdirs.end(); cit++)
			{
				ContentTreeNode* child = cit->second;
				numItems += CollectFileList(child, targetlist, totalSize, recursive);
			} //for
		}

		// Iterate through files
		for (SubNodesMap::const_iterator cit = node->files.begin(); cit != node->files.end(); cit++)
		{
			ContentTreeNode* child = cit->second;
			targetlist.push_back(child->storageIndex);
			numItems++;
			totalSize += child->GetSize();
		} //for
	}
	else
	{
		targetlist.push_back(node->storageIndex);
		numItems++;
		totalSize += node->GetSize();
	}

	return numItems;
}
