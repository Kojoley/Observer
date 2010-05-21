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
#include "cat.h"

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

typedef uintptr_t X2FILE;

/************************************************
 * X2FD_GetLastError
 *
 * Return last error code. Every interface function sets this code to 0 (no
 * error) or to one of the error constants.
 *
 * in: none
 * ret: error code
 ************************************************/
int X2FD_GetLastError();

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
 * ret: number of copied bytes or -1 if error code is invalid
 ************************************************/
X2FDULONG X2FD_TranslateError(int nErrorCode, char *pszBuffer, X2FDLONG nBuffSize, X2FDLONG *pNeeded);

/* files */

/************************************************
 * X2FD_OpenFile
 *
 * Opens a file and return its handle. Works like WINAPI CreateFile function.
 * You can open file more that once for reading, but you cannot open file for
 * reading if is already opened for writing and you cannot open it for writing
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
 * This function can open physical files from OS filesystem, transparently open
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
X2FILE X2FD_OpenFile(const char *pszName, int nAccess, int nCreateDisposition, int nFileType);

/************************************************
 * X2FD_CloseFile
 *
 * Close handle from previous call to X2FD_OpenFile. Does not necessarily close
 * the physical file itself if it has been opened more than once.
 * If the file is opened for writing and it is PCK or it is opened from CAT,
 * then the data will be saved to the file while the last reference to such file
 * is freed.
 *
 * in: hFile - handle to close
 * ret: non zero if file has been really closed and was saved ok, if the file
 *      was not closed or there was an error while saving, zero is returned
 ************************************************/
int X2FD_CloseFile(X2FILE hFile);

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
X2FDLONG X2FD_FileSize(X2FILE hFile);

/************************************************
 * X2FD_EOF
 *
 * Check if file pointer is at the file end (beyond last readable position)
 *
 * in: handle of file
 * ret: non zero if EOF is true, 0 otherwise
 ************************************************/
int X2FD_EOF(X2FILE hFile);

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
X2FDLONG X2FD_ReadFile(X2FILE hFile, void *buffer, X2FDULONG size);

/************************************************
 * X2FD_SeekFile
 *
 * Works as C runtime function fseek()
 * Will set the internal file pointer at specified location.
 *
 * in: hFile - file to seek, offset - offset in bytes from origin
 *     origin - X2FD_SEEK_SET - beginning of file, X2FD_SEEK_CUR - current
 *              position, X2FD_SEEK_END - end of file
 * ret: new offset or -1 on failure
 ************************************************/
X2FDULONG X2FD_SeekFile(X2FILE hFile, int offset, int origin);

/************************************************
 * X2FD_FileStatByHandle
 *
 * Will return X2FILEINFO structure with information about file type
 * (PCK/PLAIN), size and modification time
 *
 * in: handle to file, pointer to structure to get the info
 * ret: zero on failure, non zero otherwise
 ************************************************/
int X2FD_FileStatByHandle(X2FILE hFile, X2FILEINFO *pInfo);

/************************************************
 * X2FD_GetFileCompressionType
 *
 * Returns the compression format used in file.
 * Test is based on reading GZIP/PCK header from file.
 *
 * in: name of file
 * ret: one of X2FD_FILETYPE_ constants (on error PLAIN is returned)
 ************************************************/
int X2FD_GetFileCompressionType(const char *pszFileName);

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
x2catalog* X2FD_OpenCatalog(const char *pszName);

/************************************************
 * X2FD_CloseCatalog
 *
 * Will close handle associated with catalog, but not necessarily the catalog
 * itself if it's opened by another call to X2FD_OpenCatalog or X2FD_OpenFile
 *
 * If some file was modified inside the catalog, the CAT file (the index file
 * with extension .cat) will be overwritten upon closing to match to order of
 * files in the modified DAT file.
 *
 * There is no way to verify that it was successful.
 *
 * in: catalog to close
 * ret: 0 if handle is 0 - otherwise non zero
 ************************************************/
int X2FD_CloseCatalog(x2catalog* hCat);

/************************************************
 * X2FD_SetFileTime
 *
 * change the last modification, last access and creation time on a file
 * (for real files) and modification time (for files inside archives)
 *
 * in: file handle, time as time_t
 * ret: 0 on failure, nonzero in success
 ************************************************/
int X2FD_SetFileTime(X2FILE hFile, X2FDLONG mtime);

/************************************************
 * X2FD_CopyFile
 *
 * will move data from one file to another
 *
 * in: source file, destination file
 * ret: 0 on failure, nonzero otherwise
 ************************************************/
int X2FD_CopyFile(X2FILE hSource, X2FILE hDestination);

X2FILE X2FD_OpenFileInCatalog(const x2catalog* catalog, x2catentry* entry, int nFileType);

#endif // !defined(X2_FILE_DRIVER_INCLUDED)
