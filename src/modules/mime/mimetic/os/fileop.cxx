/***************************************************************************
    copyright            : (C) 2002-2008 by Stefano Barbato
    email                : stefano@codesink.org

    $Id: fileop.cxx,v 1.5 2008-10-07 11:06:26 tat Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <mimetic/os/fileop.h>
#include <mimetic/libconfig.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

using namespace std;

namespace mimetic
{

//static
bool FileOp::remove(const string& fqn)
{
    return _unlink(fqn.c_str()) == 0;
}

//static
bool FileOp::move(const string& oldf, const string& newf)
{
#if defined(CONFIG_UNIX)
    if(link(oldf.c_str(), newf.c_str()) == 0)
    {
        unlink(oldf.c_str());
        return true;
    }
    return false;
#elif defined(CONFIG_WIN32)
  return(rename(oldf.c_str(), newf.c_str()) == 0);
#else
#error sys not supported
#endif
}
//static
bool FileOp::exists(const string& fqn)
{
    struct stat st;
    return ::stat(fqn.c_str(), &st) == 0;
}

//static
uint FileOp::size(const string& fqn)
{
    struct stat st;
    if(::stat(fqn.c_str(), &st) == 0)
        return st.st_size;
    else
        return 0;
}

//static
uint FileOp::ctime(const string& fqn)
{
    struct stat st;
    if(::stat(fqn.c_str(), &st) == 0)
        return st.st_ctime;
    else
        return 0;
}

//static
uint FileOp::atime(const string& fqn)
{
    struct stat st;
    if(::stat(fqn.c_str(), &st) == 0)
        return st.st_atime;
    else
        return 0;
}

//static
uint FileOp::mtime(const string& fqn)
{
    struct stat st;
    if(::stat(fqn.c_str(), &st) == 0)
        return st.st_mtime;
    else
        return 0;
}

}


