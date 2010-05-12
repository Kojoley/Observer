/*
 x2fd.h

 Main header for x2fd.dll library

 Version: 1.0.0.16

 Credits: writen by doubleshadow (doubleshadow@volny.cz)
          PCK and CAT reading algorithms were taken and modified from
          assembler codes provided to me by Laga Mahesa (aka StoneD)
*/

#if !defined(X2_FILE_DRIVER_INCLUDED)
#define X2_FILE_DRIVER_INCLUDED

#include <stddef.h>

typedef intptr_t X2FDLONG; // 32bits on W32, 64 bits on W64, signed
typedef uintptr_t X2FDULONG; // 32bits on W32, 64 bits on W64, unsigned

#define X2FDEXPORT(ret) ret __stdcall

// open mode
#define X2FD_READ           0
#define X2FD_WRITE          1

// create disposition
#define X2FD_OPEN_EXISTING  0
#define X2FD_CREATE_NEW     1
#define X2FD_CONVERT        2

// seek modes
#define X2FD_SEEK_SET   0
#define X2FD_SEEK_END   1
#define X2FD_SEEK_CUR   2

// open flags
#define X2FD_FILETYPE_AUTO    0
#define X2FD_FILETYPE_PLAIN   1
#define X2FD_FILETYPE_PCK     2
#define X2FD_FILETYPE_DEFLATE 3

// errors
#define X2FD_E_MSG                (-1)
#define X2FD_E_OK                   0
#define X2FD_E_HANDLE               1
#define X2FD_E_SEEK                 2
#define X2FD_E_BAD_SEEK_SPEC        3

#define X2FD_E_FILE_ERROR           4
#define X2FD_E_FILE_ACCESS          5
#define X2FD_E_FILE_EXIST           6
#define X2FD_E_FILE_NOENTRY         7

#define X2FD_E_FILE_BADMODE         8
#define X2FD_E_FILE_BADDATA         9
#define X2FD_E_FILE_EMPTY          10
#define X2FD_E_FILE_LOCKED         11

#define X2FD_E_CAT_NOTOPEN         12
#define X2FD_E_CAT_NOENTRY         13
#define X2FD_E_CAT_NODAT           14
#define X2FD_E_CAT_INVALIDSIZE     15
#define X2FD_E_CAT_FILESLOCKED     16
#define X2FD_E_CAT_INVALIDFILENAME 17

#define X2FD_E_ERROR               18
#define X2FD_E_MALLOC              19
#define X2FD_E_BAD_FLAGS           20

#define X2FD_GZ_BEGIN              21

// GZ errors
#define X2FD_E_GZ_FLAGS         X2FD_GZ_BEGIN     // reserved bits are set
#define X2FD_E_GZ_HEADER        X2FD_GZ_BEGIN + 1 // no gzip header
#define X2FD_E_GZ_TOOSMALL      X2FD_GZ_BEGIN + 2 // input stream is too small
#define X2FD_E_GZ_CRC           X2FD_GZ_BEGIN + 3 // data crc does not match
#define X2FD_E_GZ_HCRC          X2FD_GZ_BEGIN + 4 // header crc does not match
#define X2FD_E_GZ_COMPRESSION   X2FD_GZ_BEGIN + 5 // unsupported compression
#define X2FD_E_GZ_DEFLATE       X2FD_GZ_BEGIN + 6 // deflate failed

// flags from X2FILEINFO
#define X2FD_FI_FILE        1
#define X2FD_FI_PLAIN       2
#define X2FD_FI_PCK         4
#define X2FD_FI_DEFLATE     8

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t X2FILE;
typedef uintptr_t X2CATALOG;
typedef uintptr_t X2FIND;

// used by X2FD_CatFindFirst/Next
struct X2CATFILEINFO
{
    char szFileName[260];
    size_t size;
};

// used by X2FD_FindFirst/Next
struct X2FILEINFO
{
    char szFileName[260];
    X2FDLONG mtime;
    X2FDLONG size;
    X2FDLONG BinarySize;
    int flags;
};

/************************************************
 * X2FD_GetLastError
 *
 * Return last error code. Every interface function sets this code to 0 (no
 * error) or to one of the error constants.
 *
 * in: none
 * ret: error code
 ************************************************/
X2FDEXPORT(int) X2FD_GetLastError();

/************************************************
 * X2FD_TranslateError
 *
 * Translates error from X2FD_GetLastError() to string.
 * You can call it with pszBuffer=NULL and nBufferSize=0 to get the required
 * buffer size
 * and then again to receive the text.
 *
 * in: nErrorCode - the error code, pszBuffer - buffer that will receive the
 *     string, nBufferSize - size of the buffer in bytes, pNeeded - pointer to
 *     int to get the required buffer size (including the terminating zero)
 * ret: number of copyied bytes or -1 if error code is invalid
 ************************************************/
X2FDEXPORT(X2FDULONG) X2FD_TranslateError(int nErrorCode, char *pszBuffer,
X2FDLONG nBuffSize, X2FDLONG *pNeeded);

/* files */

/************************************************
 * X2FD_OpenFile
 *
 * Opens a file and return its handle. Works like WINAPI CreateFile function.
 * You can open file more that once for reading, but you cannot open file for
 * reading if is already opened for writing and you cannot opend it for writing
 * if it's already opened for reading.
 * Every handle opened with X2FD_OpenFile must be closed with call to
 * X2FD_CloseFile
 *
 * nFileType means how X2FD will treat the file:
 * AUTO means it will check automatically if it's a PCK
 * !!! Don't use this when creating new file!!!
 * It would be always created as PLAIN.
 *
 * PLAIN means that there is no translation and you can open even PCKs with it,
 * but you will read and write the raw binary data.
 *
 * PCK means that the file is PCK. If file doesn't exist or it's 0 bytes long,
 * it will be created and treated as PCK. If file exists and contains data, then
 * it must really be a PCK file.
 *
 * This function can open physical files from OS filesystem, trasparently open
 * PCK files and transparently open files within CAT archives.
 *
 * To open file within a CAT archive use this syntax:
 * <CAT file name>::<file inside CAT>
 *
 * Example: C:\\01.cat::types\\TShips.pck will open file "types\TShips.pck"
 * from catalog "c:\01.cat".
 *
 * File name is case insensitive.
 *
 * Files opened for reading are locked for writing, files opened for writing are
 * locked for both read and write, so no other process can access them.
 *
 * in: pszName - name of file, nAccess - desired access X2FD_READ, X2FD_WRITE
 *     nCreateDisposition - whether file should be created if it does not exist
 *     (X2FD_OPEN_EXISTING - file must exist, X2FD_CREATE_NEW - file is created
 *     if does not exist, otherwise it's opened)
 *     nFileType - type of file (X2FD_FILETYPE_AUTO, X2FD_FILETYPE_PLAIN,
 *                 X2FD_FILETYPE_PCK)
 * ret: handle of opened file or 0 on failure
 ************************************************/
X2FDEXPORT(X2FILE) X2FD_OpenFile(const char *pszName, int nAccess, int
nCreateDisposition, int nFileType);

/************************************************
 * X2FD_OpenFileConvert
 *
 * Operates like OpenFile but opened file is converted to specified file type
 * File is always opened with X2FD_WRITE access
 *
 * in: pszName - name of file, nAccess - desired access X2FD_READ, X2FD_WRITE
 *     nCreateDisposition - whether file should be created if it does not exist
 *     (X2FD_OPEN_EXISTING, X2FD_CREATE_NEW)
 *     nFileType - type of file (X2FD_FILETYPE_AUTO, X2FD_FILETYPE_PLAIN,
 *                 X2FD_FILETYPE_PCK)
 *     nConvertToFileType - desired type of file (X2FD_FILETYPE_PLAIN,
 *                          X2FD_FILETYPE_PCK)
 * ret: handle of opened file or 0 on failure
 ************************************************/
X2FDEXPORT(X2FILE) X2FD_OpenFileConvert(const char *pszName, int
nCreateDisposition, int nFileType, int nConvertToFileType);

/************************************************
 * X2FD_CloseFile
 *
 * Close handle from previous call to X2FD_OpenFile. Does not neccessarily close
 * the physical file itself if it has been opened more than once.
 * If the file is opened for writing and it is PCK or it is opened from CAT,
 * then the data will be saved to the file while the last reference to such file
 * is freed.
 *
 * in: hFile - handle to close
 * ret: non zero if file has been really closed and was saved ok, if the file
 *      was not closed or there was an error while saving, zero is returned
 ************************************************/
X2FDEXPORT(int) X2FD_CloseFile(X2FILE hFile);

/************************************************
 * X2FD_FileSize
 *
 * Return size of file's buffer. For files opened as PLAIN the returned size is
 * same as physical file size. For PCK files, it's the size of data it PCK file
 * not the physical size!
 *
 * in: handle of file
 * ret: size of file or -1 on failure
 ************************************************/
X2FDEXPORT(X2FDLONG) X2FD_FileSize(X2FILE hFile);

/************************************************
 * X2FD_EOF
 *
 * Check if file pointer is at the file end (beyond last readable position)
 *
 * in: handle of file
 * ret: non zero if EOF is true, 0 otherwise
 ************************************************/
X2FDEXPORT(int) X2FD_EOF(X2FILE hFile);

/************************************************
 * X2FD_ReadFile
 *
 * Works like C runtime function fread()
 * It will read up to <size> bytes from specified file and write them to
 * <buffer>.
 * If file is opened as PCK then the data readed are also unpacked.
 * The reading will begin at location specified by internal file pointer and
 * this pointer will be shifted by number of bytes readed.
 *
 * in: hFile - file to read from, buffer - buffer to hold the readed data
 *     size - number of bytes to read from file
 * ret: number of bytes readed or -1 on error
 ************************************************/
X2FDEXPORT(X2FDLONG) X2FD_ReadFile(X2FILE hFile, void *buffer,
X2FDULONG size);

/************************************************
 * X2FD_SeekFile
 *
 * Works as C runtime function fseek()
 * Will set the internal file pointer at specified location.
 *
 * in: hFile - file to seek, offset - offset in bytes from origin
 *     origin - X2FD_SEEK_SET - begining of file, X2FD_SEEK_CUR - current
 *              position, X2FD_SEEK_END - end of file
 * ret: new offset or -1 on failure
 ************************************************/
X2FDEXPORT(X2FDULONG) X2FD_SeekFile(X2FILE hFile, int offset,
int origin);

/************************************************
 * X2FD_FileTell
 *
 * in: hFile - file
 * ret: internal pointer position or -1 on failure
 ************************************************/
X2FDEXPORT(X2FDULONG) X2FD_FileTell(X2FILE hFile);

/************************************************
 * X2FD_SetEndOfFile
 *
 * Works like WINAPI SetEndOfFile() function
 * Will enlarge/shrink the file size so its end will be at location specified by
 * internal file pointer.
 *
 * in: handle of file to change
 * ret: 0 on error, non zero on success
 ************************************************/
X2FDEXPORT(int) X2FD_SetEndOfFile(X2FILE hFile);

/************************************************
 * X2FD_WriteFile
 *
 * Works like C function fwrite()
 * Will write nCount bytes from pData to file, begining at location specified by
 * internal file pointer.
 *
 * This function will not write anything to physical file if it's PCK or it's
 * opneded from CAT archive.
 * Such file will be saved with last call to X2FD_CloseFile.
 *
 * in: hFile - file to write to, pData - data to write, nCount- count of bytes
 *     to write
 * ret: number of bytes written or -1 on failure
 ************************************************/
X2FDEXPORT(X2FDLONG) X2FD_WriteFile(X2FILE hFile, const void *pData,
X2FDULONG nCount);

/************************************************
 * X2FD_FileStat
 *
 * Will return X2FILEINFO structure with information about file type
 * (PCK/PLAIN), size and modifiation time.
 *
 * in: name of file, pointer to structure to get the info
 * ret: zero on failure, non zero otherwise
 ************************************************/
X2FDEXPORT(int) X2FD_FileStat(const char *pszFileName, X2FILEINFO
*pInfo);

/************************************************
 * X2FD_FileStatByHandle
 *
 * Will return X2FILEINFO structure with information about file type
 * (PCK/PLAIN), size and modifiation time
 *
 * in: handle to file, pointer to structure to get the info
 * ret: zero on failure, non zero otherwise
 ************************************************/
X2FDEXPORT(int) X2FD_FileStatByHandle(X2FILE hFile, X2FILEINFO *pInfo);

/************************************************
 * X2FD_FileExists
 *
 * Check if file exists. Use the same syntax as in X2FD_OpenFile to check file
 * within CAT.
 * Does not make difference if target is file or directory
 *
 * in: file name
 * ret: zero if file doesn't exist, non zero otherwise
 ************************************************/
X2FDEXPORT(int) X2FD_FileExists(const char *pszFileName);

/************************************************
 * X2FD_GetFileCompressionType
 *
 * Returns the compression format used in file.
 * Test is based on reading GZIP/PCK header from file.
 *
 * in: name of file
 * ret: one of X2FD_FILETYPE_ constants (on error PLAIN is returned)
 ************************************************/
X2FDEXPORT(int) X2FD_GetFileCompressionType(const char *pszFileName);

/************************************************
 * X2FD_DeleteFile
 *
 * Delete a file. It can be PCK, PLAIN or from catalog.
 * Use the same syntax as in X2FD_OpenFile to delete file from catalog.
 * Catalog does not need to be open in order to delete from it.
 *
 * in: file name
 * ret: zero if file doesn't exist or cannot be deleted, non zero otherwise
 ************************************************/
X2FDEXPORT(int) X2FD_DeleteFile(const char *pszFileName);

/************************************************
 * X2FD_MoveFile
 *
 * Moves a file from one destination to another
 * Currently supports only moving files within catalog or across catalogs
 * but does not support moving on real file system or between real and cat
 * filesystem
 *
 * in: file name, new name where to move
 * ret: zero on failure, non zero otherwise
 ************************************************/
X2FDEXPORT(int) X2FD_MoveFile(const char *pszFileName,
const char *pszNewName);

/* catalogs */

/************************************************
 * X2FD_OpenCatalog
 *
 * Opens catalog (with extension CAT) or create new one and return its handle.
 * Every handle opened with this function must be closed with call to
 * X2FD_CloseCatalog.
 *
 * You do not need to open catalog prior to opening file from it. It's done
 * automatically. However you must use this function when creating new catalog.
 *
 * Once the catalog is open, both its files (.cat and .dat) are exclusively
 * locked so no other processes can access them.
 *
 * in: pszName - catalog name, nCreateDisposition - whether catalog should be
 *     created if it does not exist (X2FD_OPEN_EXISTING - catalog must already
 *     exist, X2FD_CREATE_NEW - catalog will be created if it doesn't exist,
 *     otherwise it's opened)
 * ret: catalog handle or 0 on failure
 ************************************************/
X2FDEXPORT(X2CATALOG) X2FD_OpenCatalog(const char *pszName,
int nCreateDisposition);

/************************************************
 * X2FD_CloseCatalog
 *
 * Will close handle associated with catalog, but not neccessarily the catalog
 * itself if it's opened by another call to X2FD_OpenCatalog or X2FD_OpenFile
 *
 * If some file was modified inside the catalog, the CAT file (the index file
 * with extension .cat) will be overwriten upon closing to match to order of
 * files in the modified DAT file.
 *
 * There is no way to verify that it was sucessfull.
 *
 * in: catalog to close
 * ret: 0 if handle is 0 - otherwise non zero
 ************************************************/
X2FDEXPORT(int) X2FD_CloseCatalog(X2CATALOG hCat);

/************************************************
 * X2FD_DeleteCatalog
 *
 * Delete given catalog (both .cat and .dat file)
 *
 * in: catalog to delete
 * ret: 0 if catalog cannot be deleted (is opened for example), non-zero
 *      otherwise
 ************************************************/
X2FDEXPORT(int) X2FD_DeleteCatalog(const char *pszFileName);

/************************************************
 * X2FD_TimeStampToLocalTimeStamp
 *
 * Convert time stamp to local time stamp
 * If input is -1, output is also -1
 *
 * in: timestamp
 * ret: timestamp converted to localtime (or -1)
 ************************************************/
X2FDEXPORT(X2FDLONG) X2FD_TimeStampToLocalTimeStamp(X2FDLONG
TimeStamp);

/************************************************
 * X2FD_TimeStampToOLEDateTime
 *
 * Convert time stamp to OLE DataTime (Variant Date)
 * If input is -1, output is 0
 *
 * in: timestamp
 * ret: converted time
 ************************************************/
X2FDEXPORT(double) X2FD_TimeStampToOLEDateTime(unsigned long TimeStamp);

/************************************************
 * X2FD_SetFileTime
 *
 * change the last modification, last access and creation time on a file
 * (for real files) and modification time (for files inside archives)
 *
 * in: file handle, time as time_t
 * ret: 0 on failure, nonzero in success
 ************************************************/
X2FDEXPORT(int) X2FD_SetFileTime(X2FILE hFile, X2FDLONG mtime);

/************************************************
 * X2FD_CatFindFirstFile
 *
 * works like API FindFirstFile but within catalog
 * it will find all occurencies of file pszFileName in catalog
 *
 * you can use wildcards (* ?) in file name the same way as in
 * API FindFirstFile
 *
 * in: catalog, file name to look for, address of strucure for info
 * ret: search handle for use with X2FD_CatFindNextFile and X2FD_CatFindClose
 *      0 if no file name was found, nonzero otherwise
 ************************************************/
X2FDEXPORT(X2FIND) X2FD_CatFindFirstFile(X2CATALOG hCat,
const char *pszFileName, X2CATFILEINFO *pInfo);

/************************************************
 * X2FD_CatFindNextFile
 *
 * works like API FindNextFile but within catalog
 * it will find next occurence of file specified with call to
 * X2FD_CatFindFirstFile
 *
 * in: search handle returned by X2FD_CatFindFirstFile, address of strucure for
 *     info
 * ret: 0 if no more occurences were found, nonzero if there are more
 ************************************************/
X2FDEXPORT(int) X2FD_CatFindNextFile(X2FIND hFind, X2CATFILEINFO *pInfo);

/************************************************
 * X2FD_CatFindClose
 *
 * works like API FindClose
 * it will delete all data associated with given search handle
 * you cannot call any search functions with this handle after that
 *
 * in: search handle returned by X2FD_CatFindFirst
 * ret: 0 on failure, nonzero otherwise
 ************************************************/
X2FDEXPORT(int) X2FD_CatFindClose(X2FIND hFind);

/************************************************
 * X2FD_CopyFile
 *
 * will move data from one file to another
 *
 * in: source file, destination file
 * ret: 0 on failure, nonzero otherwise
 ************************************************/
X2FDEXPORT(int) X2FD_CopyFile(X2FILE hSource, X2FILE hDestination);

#ifdef __cplusplus
}
#endif

#endif // !defined(X2_FILE_DRIVER_INCLUDED)
