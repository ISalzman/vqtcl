/* V9 segment memory allocation
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

// typedef struct FixedVector {
//     const char* string;
// } FixedVector;

// typedef struct DynamicVector {
//     char* buffer;
//     long limit;
// } DynamicVector;

typedef struct MappedVector {
    char* memory;
} MappedVector;

typedef struct SharedVector {
    void* aux[3]; // must be first member
    Vector reference;
    long offset;
} SharedVector;

Vector IncRef (Vector vp) {
    ++Head(vp).shares;
    return vp;
}

Vector DecRef (Vector vp) {
    if (vp != 0 && --Head(vp).shares < 0) {
		if (Head(vp).type->cleaner != 0)
			Head(vp).type->cleaner(vp);
        Head(vp).alloc->release(vp);
        return 0;
    }
    return vp;
}

void Unshare (Vector* vpp) {
    if (IsShared(*vpp)) {
        assert(Head(*vpp).type->duplicator != 0);
        Vector vnew = Head(*vpp).type->duplicator(*vpp);
        DecRef(*vpp);
        *vpp = IncRef(vnew);
    }
}

static void InlineVectorCleaner (Vector vp) {
    free(vp-1);
}

// static void FixedVectorCleaner (Vector vp) {
//     free(vp-1);
// }

// static void DynamicVectorCleaner (Vector vp) {
//     DynamicVector* dvp = (DynamicVector*) vp;
//     free(dvp->buffer);
//     free(vp-1);
// }

static void MappedVectorCleaner (Vector vp) {
    MappedVector* mvp = (MappedVector*) vp;
#ifdef WIN32
    UnmapViewOfFile(mvp->memory);
#else
    munmap(mvp->memory, Head(vp).count);
#endif
    free(vp-1);
}

static void SharedVectorCleaner (Vector vp) {
    SharedVector* ovp = (SharedVector*) vp;
    DecRef(ovp->reference);
    free(vp-1);
}

static const char* InlineVectorAddress (Vector vp) {
    return (const char*) vp;
}

// static const char* FixedVectorAddress (Vector vp) {
//     FixedVector* svp = (FixedVector*) vp;
//     return svp->string;
// }

// static const char* DynamicVectorAddress (Vector vp) {
//     DynamicVector* dvp = (DynamicVector*) vp;
//     return dvp->buffer;
// }

static const char* MappedVectorAddress (Vector vp) {
    MappedVector* mvp = (MappedVector*) vp;
    return mvp->memory;
}

static const char* SharedVectorAddress (Vector vp) {
    SharedVector* ovp = (SharedVector*) vp;
    return (const char*) Address(ovp->reference) + ovp->offset;
}

static VectorAlloc inlineVectorAlloc = {
    "inline",
    InlineVectorCleaner,
    InlineVectorAddress,
};

// static VectorAlloc fixedVectorAlloc = {
//     "fixed",
//     FixedVectorCleaner,
//     FixedVectorAddress,
// };

// static VectorAlloc dynamicVectorAlloc = {
//     "dynamic",
//     DynamicVectorCleaner,
//     DynamicVectorAddress,
// };

static VectorAlloc mappedVectorAlloc = {
    "mapped",
    MappedVectorCleaner,
    MappedVectorAddress,
};

static VectorAlloc sharedVectorAlloc = {
    "overlap",
    SharedVectorCleaner,
    SharedVectorAddress,
};

static Vector AllocVec(VectorAlloc* va, int bytes) {
    Vector newvp = 1 + (Vector) malloc(sizeof *newvp + bytes);
    Head(newvp).alloc = va;
    Head(newvp).type = 0;
    Head(newvp).count = 0;
    Head(newvp).shares = -1;
    return newvp;
}

Vector NewInlineVector (long length) {
    Vector newvp = AllocVec(&inlineVectorAlloc, length);
    Head(newvp).count = length;
	memset(newvp, 0, length);
    return newvp;
}

// Vector NewFixedVector (const char* ptr, long length) {
//     Vector newvp = AllocVec(&fixedVectorAlloc, sizeof(FixedVector));
//     Head(newvp).count = length;
//     
//     FixedVector* svp = (FixedVector*) newvp;
//     svp->string = ptr;
//     
//     return newvp;
// }

// Vector NewDynamicVector () {
//     Vector newvp = AllocVec(&dynamicVectorAlloc, sizeof(DynamicVector));
//     
//     DynamicVector* dvp = (DynamicVector*) newvp;
//     dvp->buffer = 0;
//     dvp->limit = 0;
//     
//     return newvp;
// }

Vector NewMappedVector (int fd) {
    char* data = 0;
    int length = -1;
    
#ifdef WIN32
    // HANDLE fd = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0,
    //                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    HANDLE h = CreateFileMapping(f, 0, PAGE_READONLY, 0, 0, 0);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD n = GetFileSize(f, 0);
        data = MapViewOfFile(h, FILE_MAP_READ, 0, 0, n);
        if (data != NULL)
            length = AuthenticodeOffset((LPBYTE)data, n);
        CloseHandle(h);
    }
#else    
    // int fd = open(filename, O_RDONLY);
    struct stat sb;
    fstat(fd, &sb);
    length = sb.st_size;
    data = mmap(0, sb.st_size, PROT_READ, 0, fd, 0);
#endif

    Vector newvp = AllocVec(&mappedVectorAlloc, sizeof(MappedVector));
    Head(newvp).count = length;

    MappedVector* mvp = (MappedVector*) newvp;
    mvp->memory = data;

    return newvp;
}

Vector NewSharedVector (Vector vp, long offset, long length) {
    Vector newvp = AllocVec(&sharedVectorAlloc, sizeof(SharedVector));
    Head(newvp).count = length;

    SharedVector* ovp = (SharedVector*) newvp;
    ovp->reference = IncRef(vp);
    ovp->offset = offset;
    
    return newvp;
}
