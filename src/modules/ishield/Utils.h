#ifndef Utils_h__
#define Utils_h__

void CombinePath(char* buffer, size_t bufferSize, int numParts, ...);
void CombinePath(wchar_t* buffer, size_t bufferSize, int numParts, ...);

DWORD GetMajorVersion(DWORD fileVersion);
std::wstring GenerateCabPatern(const wchar_t* headerFileName);

std::wstring ConvertStrings(std::string &input);

void DecryptBuffer(BYTE* buf, DWORD bufSize, DWORD* pTotal);
bool UnpackBuffer(BYTE* inBuf, size_t inSize, BYTE* outBuf, size_t* outBufferSize, size_t* outDataSize, bool blockStyle);

uint8_t* find_bytes(const uint8_t* buffer, size_t bufferSize, const uint8_t* pattern, size_t patternSize);

#endif // Utils_h__
