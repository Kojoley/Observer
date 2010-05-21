#include "StdAfx.h"
#include "common/ext_list.h"
#include "common/strutils.h"
#include "x2fd.h"
#include "cat.h"
#include "files.h"
#include "catpck.h"
#include "local_error.h"

#define getfile(handle) (xfile*) handle

typedef ext::list<filebuffer*> bufferlist;

bufferlist g_bufflist;

//---------------------------------------------------------------------------------
bool checkBuffMode(filebuffer *b, int mode)
{
	if(mode!=X2FD_READ && mode!=X2FD_WRITE) return false;
	return !((mode==X2FD_WRITE) || (b->locked_w() > 0 && (mode==X2FD_READ)));
}
//---------------------------------------------------------------------------------
filebuffer* findBuffer(const char *pszName)
{
	bufferlist::iterator it;

	for(it=g_bufflist.begin(); it!=g_bufflist.end(); ++it)	{
		if(_stricmp(it->pszName, pszName)==0)
			break;
	}
	if(it!=g_bufflist.end()){
		it->addref();
		return *it;
	}
	else
		return NULL;
}
//---------------------------------------------------------------------------------
X2FILE OpenFile(const char *pszName, int nAccess, int nCreateDisposition, int nFileType)
{
	filebuffer *buff;

	clrerr();
	if(nFileType == X2FD_FILETYPE_AUTO)
		nFileType = GetFileCompressionType(pszName);

	buff=findBuffer(pszName);
	if(buff){
		// check the filemode
		if(checkBuffMode(buff, nAccess)==false){
			error(X2FD_E_FILE_BADMODE);
			buff->release();
			return 0;
		}
	}
	// plain files - we will always open new filebuffer
	if(nFileType==X2FD_FILETYPE_PLAIN){
		if(buff) buff->release();
		buff=NULL;
	}

	if(buff==NULL){
		buff=new filebuffer();
		bool bRes=buff->openFile(pszName, nAccess, nCreateDisposition, nFileType);
		error(buff->error()); // always set the error
		if(bRes==false){
			buff->release();
			buff=NULL;
		}
		else
			g_bufflist.push_back(buff);
	}

	xfile *f;
	if(buff){
		f=new xfile();
		f->buffer(buff);
		f->mode(nAccess);
		buff->release();
	}
	else
		f=NULL;

	return (f==NULL ? NULL : (X2FILE) f);
}
//---------------------------------------------------------------------------------
X2FILE X2FD_OpenFile(const char *pszName, int nAccess, int nCreateDisposition, int nFileType)
{
	clrerr();
	// check the flags so we can rely on them later
	if(nAccess != X2FD_READ && nAccess != X2FD_WRITE){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}
	if(nCreateDisposition != X2FD_CREATE_NEW && nCreateDisposition != X2FD_OPEN_EXISTING){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}
	if(nFileType != X2FD_FILETYPE_PCK && nFileType != X2FD_FILETYPE_DEFLATE && nFileType != X2FD_FILETYPE_PLAIN && nFileType != X2FD_FILETYPE_AUTO){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}
	if(nCreateDisposition == X2FD_CREATE_NEW && nAccess != X2FD_WRITE){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}

	X2FILE f = OpenFile(pszName, nAccess, nCreateDisposition, nFileType);
	
	return f;
}
//---------------------------------------------------------------------------------
X2FILE X2FD_OpenFileInCatalog(const x2catalog* catalog, x2catentry* entry, int nFileType)
{
	clrerr();
	if(nFileType != X2FD_FILETYPE_PCK && nFileType != X2FD_FILETYPE_DEFLATE && nFileType != X2FD_FILETYPE_PLAIN && nFileType != X2FD_FILETYPE_AUTO){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}

	// look if buffer is already open
	filebuffer *buff=findBuffer(entry->pszFileName);
	if(buff!=NULL){
		if(checkBuffMode(buff, X2FD_READ)==false){
			buff->release();
			buff=NULL;
			error(X2FD_E_FILE_BADMODE);
		}
	}
	// we must open it
	else{
		x2catbuffer *catbuff = catalog->buffer();
		if (catbuff!=NULL)
		{
			buff=catbuff->loadFile(entry, nFileType);

			if(buff)
				g_bufflist.push_back(buff);
			else
				error(catbuff->error());
		}
	}

	xfile *f;
	if(buff){
		f=new xfile();
		f->buffer(buff);
		f->mode(X2FD_READ);
		buff->release();
	}
	else
		f=NULL;

	return (f==NULL ? NULL : (X2FILE) f);
}
//---------------------------------------------------------------------------------
X2FDLONG X2FD_FileSize(X2FILE hFile)
{
	xfile *f=getfile(hFile);
	X2FDLONG s=-1;

	clrerr();
	if(!f)
		error(X2FD_E_HANDLE);
	else
		s=(X2FDLONG)f->filesize();

	return s;
}
//---------------------------------------------------------------------------------
int X2FD_GetLastError()
{
	return error();
}
//---------------------------------------------------------------------------------
X2FDULONG X2FD_TranslateError(int nErrCode, char *pszBuffer, X2FDLONG nBuffSize, X2FDLONG *pNeeded)
{
	size_t m=0, errlen;
	const char *msg=errormsg(nErrCode);
	if(msg)
		errlen=strlen(msg);
	else
		errlen=0;

	if(pszBuffer && nBuffSize > 0){
		strncpy(pszBuffer, msg, m=__min((size_t)nBuffSize, errlen + 1));
		pszBuffer[m - 1]=0;
	}
	if(pNeeded) *pNeeded=errlen + 1;
	return m;
}
//---------------------------------------------------------------------------------
X2FDLONG X2FD_ReadFile(X2FILE hFile, void *buffer, X2FDULONG size)
{
	xfile *f=getfile(hFile);
	X2FDLONG r=-1;
	clrerr();
	if(f==NULL)
		error(X2FD_E_HANDLE);
	else{
		r=(X2FDLONG)f->read(buffer, size);
		if(r==-1) error(f->error());
	}
	return r;
}
//---------------------------------------------------------------------------------
int X2FD_EOF(X2FILE hFile)
{
	xfile *f=getfile(hFile);
	clrerr();
	if(f)
		return f->eof();
	else{
		error(X2FD_E_HANDLE);
		return 0;
	}
}
//---------------------------------------------------------------------------------
X2FDULONG X2FD_SeekFile(X2FILE hFile, int offset, int origin)
{
	xfile *f=getfile(hFile);
	X2FDULONG newoff = (X2FDULONG)-1;
	clrerr();
	if(f==NULL)
		error(X2FD_E_HANDLE);
	else{
		newoff=(X2FDULONG)f->seek(offset, origin);
		if(newoff==-1) error(f->error());
	}
	return newoff;
}
//---------------------------------------------------------------------------------
x2catalog* X2FD_OpenCatalog(const char *pszName)
{
	x2catalog *cat=NULL;
	x2catbuffer *cbuff;

	clrerr();

	bool bRes;

	cbuff=new x2catbuffer();
	bRes=cbuff->open(pszName);

	if(bRes==false){
		error(cbuff->error());
		cbuff->release();
		cbuff=NULL;
	}

	if(cbuff) {
		cat=new x2catalog(cbuff);
		cbuff->release();
	}

	return cat;
}
//---------------------------------------------------------------------------------
int X2FD_CloseCatalog(x2catalog* hCat)
{
	clrerr();
	if(hCat!=NULL){
		delete hCat;
		return true;
	}
	else{
		error(X2FD_E_HANDLE);
		return false;
	}
}
//---------------------------------------------------------------------------------
int X2FD_CloseFile(X2FILE hFile)
{
	xfile *f = getfile(hFile);
	bool bRes = false;

	clrerr();
	if(f)	{
		// remove and save filebuffer if it will be deleted
		if(f->buffer()->refcount() == 1){
			g_bufflist.erase(g_bufflist.find(f->buffer()));

			if(f->dirty())
				bRes = f->save();
			else
				bRes = true;

			if(bRes == false)
				error(f->error());
		}
		delete f;
	}
	return bRes;
}
//---------------------------------------------------------------------------------
int X2FD_FileStatByHandle(X2FILE hFile, X2FILEINFO *pInfo)
{
	xfile *f = getfile(hFile);
	if(f == NULL){
		error(X2FD_E_HANDLE);
		return 0;
	}

	pInfo->flags = f->buffer()->type;
	pInfo->mtime = f->buffer()->mtime();
	pInfo->size = (X2FDLONG)f->size();
	pInfo->BinarySize = (X2FDLONG)f->binarysize();
	strncpy(pInfo->szFileName, f->buffer()->pszName, sizeof(pInfo->szFileName));

	return 1;
}
//---------------------------------------------------------------------------------
int X2FD_GetFileCompressionType(const char *pszFileName)
{
	return GetFileCompressionType(pszFileName);
}
//---------------------------------------------------------------------------------
int X2FD_SetFileTime(X2FILE hFile, X2FDLONG mtime)
{
	xfile *f=getfile(hFile);
	if(f==0){
		error(X2FD_E_HANDLE);
		return false;
	}
	f->mtime(mtime);
	return true;
}
//---------------------------------------------------------------------------------
/*
	copy data from one file to another
	uses some optimizations

	both source and destination files are set to EOF after the copy
	Furthermore SetEndOfFile(destination) is called
*/
int X2FD_CopyFile(X2FILE hSource, X2FILE hDestination)
{
	xfile *src, *dest;
	src=getfile(hSource);
	dest=getfile(hDestination);
	bool bRes;

	clrerr();

	if(src==NULL || dest==NULL){
		error(X2FD_E_HANDLE);
		return false;
	}
	if(dest->mode()!=X2FD_WRITE){
		error(X2FD_E_FILE_BADMODE);
		return false;
	}

	io64::file::size size;
	unsigned char *data;
	bool srcisnative = (src->buffer()->type & filebuffer::ISFILE) && (src->buffer()->type & filebuffer::ISPLAIN);
	bool destisnative = (dest->buffer()->type & filebuffer::ISFILE) && (dest->buffer()->type & filebuffer::ISPLAIN);

	if(srcisnative){
		size=src->filesize();
		data=new unsigned char[(size_t)size];
		src->seek(0, X2FD_SEEK_SET);
		if(src->read(data, size)!=size){
			error(src->error());
			delete[] data;
			return false;
		}
	}
	else{
		data=src->buffer()->data();
		size=src->size();
		src->seek(0, X2FD_SEEK_END);
	}

	if(srcisnative && destisnative==false){
		dest->buffer()->data(data, (size_t)size, (size_t)size);
		dest->seek(0, X2FD_SEEK_END);
		dest->buffer()->dirty(true);
		data=NULL;
		bRes=true;
	}
	else{
		dest->seek(0, X2FD_SEEK_SET);
		bRes=(dest->write(data, size)==size);
		if(bRes==false)
			error(dest->error());
	}
	
	// if no mtime is available, use current time
	time_t mtime = src->mtime();
	if(mtime == -1)
		time(&mtime);
	dest->mtime(mtime);
	dest->seteof();

	if(srcisnative)
		delete[] data;

	return bRes;
}
//---------------------------------------------------------------------------------