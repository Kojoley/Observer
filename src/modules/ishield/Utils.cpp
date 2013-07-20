#include "stdafx.h"

bool ReadBuffer(HANDLE file, LPVOID buffer, DWORD bufferSize)
{
	DWORD dwBytes;
	return ReadFile(file, buffer, bufferSize, &dwBytes, NULL) && (dwBytes == bufferSize);
}

bool SeekFile(HANDLE file, DWORD position)
{
	DWORD filePtr = SetFilePointer(file, position, NULL, FILE_BEGIN);
	return (filePtr != INVALID_SET_FILE_POINTER) && (filePtr == position);
}

void CombinePath(char* buffer, size_t bufferSize, int numParts, ...)
{
	va_list argptr;
	va_start(argptr, numParts);
	for (int i = 0; i < numParts; i++)
	{
		char* next = va_arg(argptr, char*);
		if (next && *next)
		{
			if (buffer[0])
				strcat_s(buffer, bufferSize, "\\");

			strcat_s(buffer, bufferSize, next);
		}
	}
	va_end(argptr);
}

void CombinePath(wchar_t* buffer, size_t bufferSize, int numParts, ...)
{
	va_list argptr;
	va_start(argptr, numParts);
	for (int i = 0; i < numParts; i++)
	{
		wchar_t* next = va_arg(argptr, wchar_t*);
		if (next && *next)
		{
			if (buffer[0])
				wcscat_s(buffer, bufferSize, L"\\");

			wcscat_s(buffer, bufferSize, next);
		}
	}
	va_end(argptr);
}

DWORD GetMajorVersion(DWORD fileVersion)
{
	if (fileVersion >> 24 == 1)
	{
		return (fileVersion >> 12) & 0xf;
	}
	else if (fileVersion >> 24 == 2 || fileVersion >> 24 == 4)
	{
		DWORD major = (fileVersion & 0xffff);
		if (major != 0) major /= 100;
		return major;
	}

	return 0;
}

std::wstring ConvertStrings( std::string &input )
{
	int numChars = MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, NULL, 0);
	
	size_t bufSize = (numChars + 1) * sizeof(wchar_t);
	wchar_t* buf = (wchar_t*) malloc(bufSize);
	memset(buf, 0, bufSize);
	
	MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, buf, numChars + 1);
	std::wstring output(buf);
	
	free(buf);
	return output;
}

std::wstring GenerateCabPatern(const wchar_t* headerFileName)
{
	wchar_t* nameBuf = _wcsdup(headerFileName);
	
	wchar_t* dot = wcsrchr(nameBuf, L'.');
	size_t pos = dot ? dot - nameBuf : wcslen(nameBuf);
	for (pos--; isdigit(nameBuf[pos]); pos--)
	{
		nameBuf[pos] = L'\0';
	}
	nameBuf[pos+1] = L'\0';

	std::wstring result(nameBuf);
	result.append(L"%s.cab");
	
	free(nameBuf);
	return result;
}
