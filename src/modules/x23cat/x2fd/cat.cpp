#include "StdAfx.h"
#include "cat.h"
#include "files.h"
#include "catpck.h"
#include "common/strutils.h"
#include "common/gzip.h"
//---------------------------------------------------------------------------------
void x2catbuffer::fileerror(int ferr)
{  
	int e;
	switch(ferr){
		case EACCES:
			e=X2FD_E_FILE_ACCESS;
			break;
		case EEXIST:
			e=X2FD_E_FILE_EXIST;
			break;
		case ENOENT:
			e=X2FD_E_FILE_NOENTRY;
			break;
		default:
			e=X2FD_E_FILE_ERROR;
			break;
	}
	error(e);
}
//---------------------------------------------------------------------------------
bool x2catbuffer::open(const wchar_t *pszName)
{
	io64::file::size size, filesize;
	bool bRes=true;
	
	error(0);

	wstrcreate(m_pszFileName, pszName);
	m_hCATFile=io64::file::open(pszName, _O_RDWR | _O_BINARY, 0, _SH_DENYRW);
	if(!m_hCATFile.valid()) {
		fileerror(m_hCATFile.error());
		return false;
	}
	filesize=m_hCATFile.getSize();
	if(filesize==0){
		m_hCATFile.close();
		error(X2FD_E_FILE_EMPTY);
		return false;
	}
	// buffer size - must be multiple of 5 - see DecryptCAT for reason
	size=filesize + ((filesize % 5U) ? 5U - (filesize % 5U) : 0);
	//byte *buffer=new byte[size + 1];
	byte *buffer=new byte[(size_t)size + 1];
	buffer[(INT_PTR) filesize]=0;
	m_hCATFile.read(buffer, filesize);
	DecryptCAT(buffer, filesize);
	
	memrep(buffer, 13, 0, (size_t)filesize, 0);
	memrep(buffer, 10, 0, (size_t)filesize, 0);

	x2catentry *info;	
	char *pos, *old, *mid, *end;
	bool first=true;
	io64::file::size offset=0;
	pos=(char*)buffer;
	end = pos + (INT_PTR) filesize;
	do{
		old=pos;
		pos=(char*)memchr(pos, 0, end - pos);
		if(pos){
			if(first){
				size_t nNameLen = strlen(old);
				m_pszDATName = new wchar_t[nNameLen + 1];
				MultiByteToWideChar(CP_ACP, 0, old, (int) nNameLen, m_pszDATName, (int) nNameLen + 1);
				m_pszDATName[nNameLen] = 0;
				first=false;
			}
			else{
				mid=strrchr(old, ' ');
				if(mid==NULL){
					bRes=false;
					break;
				}
				mid[0]=0;
				info=new x2catentry();
				MultiByteToWideChar(CP_ACP, 0, old, (int) strlen(old), info->pszFileName, MAX_PATH);
				for (int i=0; i < (int) wcslen(info->pszFileName); i++) // Normalize path
				{
					if (info->pszFileName[i] == '/') info->pszFileName[i] = '\\';
				}
				info->offset=offset;
				info->size=atoi(++mid);
				push_back(info);
				offset+=info->size;
			}
			for( ; *pos==0 && (pos < end); pos++); // skip zeros
		}
	}
	while(pos && (pos < end));
	
	delete[] buffer;
	
	if(bRes==false)
		error(X2FD_E_FILE_BADDATA);
	else{
		// we must open the corresponding DAT file as well
		// do not use stored dat name because it is always 01.dat even for 02.cat and 03.cat

		wchar_t *path = ChangeFileExtension(pszName, L"dat");
		m_hDATFile=io64::file::open(path, _O_RDWR | _O_BINARY, 0, _SH_DENYRW);
		delete[] path;

		bRes=m_hDATFile.valid();
		if(!bRes){
			if(m_hDATFile.error()==ENOENT)
				error(X2FD_E_CAT_NODAT);
			else
				fileerror(m_hDATFile.error());
		}
	}
	return bRes;
}
//---------------------------------------------------------------------------------
filebuffer * x2catbuffer::loadFile(x2catentry *entry, int fileType)
{
	filebuffer *buff=NULL;
	byte *outdata; 
	io64::file::size outsize;
	time_t mtime=-1; // -1 mean "not set"
	
	buff=new filebuffer();
	if(entry->size==0){
		outdata=0;
		outsize=0;
		if(fileType==X2FD_FILETYPE_AUTO) fileType=X2FD_FILETYPE_PCK;
		buff->type|=fileType;
	}
	else{
		byte *data=new byte[(size_t)entry->size];
		m_hDATFile.seek(entry->offset, SEEK_SET);
		if(m_hDATFile.read(data, entry->size)!=entry->size){
			error(X2FD_E_CAT_INVALIDSIZE);
			return NULL;
		}
		
		DecryptDAT(data, entry->size);
		
		if(fileType==X2FD_FILETYPE_AUTO)
			fileType = GetBufferCompressionType(data, entry->size);
		
		// plain file
		if(fileType==X2FD_FILETYPE_PLAIN){
			outdata=data;
			outsize=entry->size;
			buff->type|=filebuffer::ISPLAIN;
		}
		// compressed
		else {
			error(DecompressBuffer(data, entry->size, fileType, &outdata, &outsize, &mtime));
			bool bErr=(error()!=0 && error()!=X2FD_E_GZ_FLAGS);
			delete[] data;
			if(bErr){
				delete buff;
				buff=NULL;
			}
			else
				buff->type|=filebuffer::fileTypeToBufferType(fileType);
		}
	}
	if(buff){
		entry->buffer=buff;
		buff->data(outdata, (size_t)outsize, (size_t)outsize);
		buff->mtime(mtime);
		buff->binarysize((size_t)entry->size);			
		wstrcreate(buff->pszName, entry->pszFileName);
	}

	return buff;
}
//---------------------------------------------------------------------------------


