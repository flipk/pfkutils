
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

const NFS_PORT          = 2049;
const NFS_MAXDATA       = 8192;
const NFS_MAXPATHLEN    = 1024;
const NFS_MAXNAMLEN	= 255;
const NFS_FHSIZE	= 32;
const NFS_COOKIESIZE	= 4;
const NFS_FIFO_DEV	= -1;	

const NFSMODE_FMT  = 0170000;	
const NFSMODE_DIR  = 0040000;	
const NFSMODE_CHR  = 0020000;	
const NFSMODE_BLK  = 0060000;	
const NFSMODE_REG  = 0100000;	
const NFSMODE_LNK  = 0120000;	
const NFSMODE_SOCK = 0140000;	
const NFSMODE_FIFO = 0010000;	

enum nfsstat {
	NFS_OK= 0,
	NFSERR_PERM=1,
	NFSERR_NOENT=2,
	NFSERR_IO=5,
	NFSERR_NXIO=6,
	NFSERR_ACCES=13,
	NFSERR_EXIST=17,
	NFSERR_NODEV=19,
	NFSERR_NOTDIR=20,
	NFSERR_ISDIR=21,
	NFSERR_FBIG=27,
	NFSERR_NOSPC=28,
	NFSERR_ROFS=30,
	NFSERR_NAMETOOLONG=63,
	NFSERR_NOTEMPTY=66,
	NFSERR_DQUOT=69,
	NFSERR_STALE=70,
	NFSERR_WFLUSH=99
};

enum ftype {
	NFNON = 0,
	NFREG = 1,
	NFDIR = 2,
	NFBLK = 3,
	NFCHR = 4,
	NFLNK = 5,
	NFSOCK = 6,
	NFBAD = 7,
	NFFIFO = 8
};

struct nfs_fh {
	opaque data[NFS_FHSIZE];
};

struct nfstime {
	unsigned seconds;
	unsigned useconds;
};

struct fattr {
	ftype type;		
	unsigned mode;		
	unsigned nlink;		
	unsigned uid;		
	unsigned gid;		
	unsigned size;		
	unsigned blocksize;	
	unsigned rdev;		
	unsigned blocks;	
	unsigned fsid;		
	unsigned fileid;	
	nfstime	atime;		
	nfstime	mtime;		
	nfstime	ctime;		
};

struct sattr {
	unsigned mode;	
	unsigned uid;	
	unsigned gid;	
	unsigned size;	
	nfstime	atime;	
	nfstime	mtime;	
};

typedef string filename<NFS_MAXNAMLEN>; 
typedef string nfspath<NFS_MAXPATHLEN>;

union attrstat switch (nfsstat status) {
case NFS_OK:
	fattr attributes;
default:
	void;
};

struct sattrargs {
	nfs_fh file;
	sattr attributes;
};

struct diropargs {
	nfs_fh	dir;	
	filename name;		
};

struct diropokres {
	nfs_fh file;
	fattr attributes;
};

union diropres switch (nfsstat status) {
case NFS_OK:
	diropokres diropres;
default:
	void;
};

union readlinkres switch (nfsstat status) {
case NFS_OK:
	nfspath data;
default:
	void;
};

struct readargs {
	nfs_fh file;		
	unsigned offset;	
	unsigned count;		
	unsigned totalcount;	
};

struct readokres {
	fattr	attributes;	
	opaque data<NFS_MAXDATA>;
};

union readres switch (nfsstat status) {
case NFS_OK:
	readokres reply;
default:
	void;
};

struct writeargs {
	nfs_fh	file;		
	unsigned beginoffset;	
	unsigned offset;	
	unsigned totalcount;	
	opaque data<NFS_MAXDATA>;
};

struct createargs {
	diropargs where;
	sattr attributes;
};

struct renameargs {
	diropargs from;
	diropargs to;
};

struct linkargs {
	nfs_fh from;
	diropargs to;
};

struct symlinkargs {
	diropargs from;
	nfspath to;
	sattr attributes;
};

typedef opaque nfscookie[NFS_COOKIESIZE];

struct readdirargs {
	nfs_fh dir;		
	nfscookie cookie;
	unsigned count;		
};

struct entry {
	unsigned fileid;
	filename name;
	nfscookie cookie;
	entry *nextentry;
};

struct dirlist {
	entry *entries;
	bool eof;
};

union readdirres switch (nfsstat status) {
case NFS_OK:
	dirlist reply;
default:
	void;
};

struct statfsokres {
	unsigned tsize;	
	unsigned bsize;	
	unsigned blocks;	
	unsigned bfree;	
	unsigned bavail;	
};

union statfsres switch (nfsstat status) {
case NFS_OK:
	statfsokres reply;
default:
	void;
};

program NFS_PROGRAM {
	version NFS_VERSION {
		void 
		NFSPROC_NULL(void) = 0;

		attrstat 
		NFSPROC_GETATTR(nfs_fh) = 1;

		attrstat 
		NFSPROC_SETATTR(sattrargs) = 2;

		void 
		NFSPROC_ROOT(void) = 3;

		diropres 
		NFSPROC_LOOKUP(diropargs) = 4;

		readlinkres 
		NFSPROC_READLINK(nfs_fh) = 5;

		readres 
		NFSPROC_READ(readargs) = 6;

		void 
		NFSPROC_WRITECACHE(void) = 7;

		attrstat
		NFSPROC_WRITE(writeargs) = 8;

		diropres
		NFSPROC_CREATE(createargs) = 9;

		nfsstat
		NFSPROC_REMOVE(diropargs) = 10;

		nfsstat
		NFSPROC_RENAME(renameargs) = 11;

		nfsstat
		NFSPROC_LINK(linkargs) = 12;

		nfsstat
		NFSPROC_SYMLINK(symlinkargs) = 13;

		diropres
		NFSPROC_MKDIR(createargs) = 14;

		nfsstat
		NFSPROC_RMDIR(diropargs) = 15;

		readdirres
		NFSPROC_READDIR(readdirargs) = 16;

		statfsres
		NFSPROC_STATFS(nfs_fh) = 17;
	} = 2;
} = 100003;
