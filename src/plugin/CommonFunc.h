#ifndef CommonFunc_h__
#define CommonFunc_h__

#define PATH_BUFFER_SIZE 4096

bool FileExists(wstring& path, LPWIN32_FIND_DATAW file_data);
bool DirectoryExists(const wchar_t* path);
bool IsDiskRoot(const wchar_t* path);
bool IsEnoughSpaceInPath(const wchar_t* path, __int64 requiredSize);
bool ForceDirectoryExist(const wchar_t* path);

bool CheckEsc();

const wchar_t* ExtractFileName(const wchar_t* fullPath);
wstring GetDirectoryName(const wstring &fullPath, bool includeTrailingDelim);
void CutFileNameFromPath(wchar_t* fullPath, bool includeTrailingDelim);

void IncludeTrailingPathDelim(wchar_t *pathBuf, size_t bufMaxSize);
void IncludeTrailingPathDelim(char *pathBuf, size_t bufMaxSize);
void IncludeTrailingPathDelim(std::wstring& pathBuf);
void IncludeTrailingPathDelim(std::string& pathBuf);

void InsertCommas(wchar_t *Dest);
void InsertCommas(char *Dest);

std::wstring FormatString(const wchar_t* fmt_str, ...);
std::wstring FileTimeToString(FILETIME ft);

void UpdateFileTime(const wchar_t* path, const FILETIME* createTime, const FILETIME* modTime);

#endif // CommonFunc_h__