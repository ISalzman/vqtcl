/*  Memory mapped files.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

/* for mmap */
#ifdef VQ_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#define MappedFile(v) ((vqMap) (v))

typedef struct vqMap_s {
    const char *data;
    size_t size;
    int parent;
    int areas;
    vqView vref;
} *vqMap;

#if 0
union vqMap_u {
    struct vqMap_s  map; /* payload */
    struct vqInfo_s info[1]; /* common header, accessed as info[-1] */
    struct vqView_s vhead[1]; /* view-specific header, accessed as vhead[-1] */
};
#endif

static const void *OpenMappedFile (const char *filename, size_t *szp) {
    const char *data = 0;
    intptr_t length = -1;
#ifdef VQ_WIN32
    {
        DWORD n;
        HANDLE h, f = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (f != INVALID_HANDLE_VALUE) {
            h = CreateFileMapping(f, 0, PAGE_READONLY, 0, 0, 0);
            if (h != INVALID_HANDLE_VALUE) {
                n = GetFileSize(f, 0);
                data = MapViewOfFile(h, FILE_MAP_READ, 0, 0, n);
                if (data != 0)
                    length = n;
                CloseHandle(h);
            }
            CloseHandle(f);
        }
    }
#else
    {
        struct stat sb;
        int fd = open(filename, O_RDONLY);
        if (fd != -1) {
            if (fstat(fd, &sb) == 0) {
                data = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
                if (data != MAP_FAILED)
                    length = sb.st_size;
            }
            close(fd);
        }
    }
#endif
    if (length < 0)
        data = 0;
    *szp = length;
    return data;
}

static void CloseMappedFile (void *data, size_t size) {
#ifdef VQ_WIN32
    UnmapViewOfFile(data);
    VQ_UNUSED(size);
#else
    munmap(data, size);
#endif
}

static void MapCleaner (vqColumn cp) {
    vqView v = cp->asview;
    vqMap map = MappedFile(v);
    if (map->parent == 0 && map->vref == 0)
        CloseMappedFile((void*) map->data, map->size);
    vq_decref(map->vref);
    lua_unref(vwEnv(v), map->parent);
    lua_unref(vwEnv(v), map->areas);
    ViewCleaner(cp);
}

/* a map can be used as a view, so refcounting them works */
static vqDispatch mapvtab = {
    "map", sizeof(struct vqView_s), 0, MapCleaner
};

static vqView new_map (vqEnv env, int t, vqView parent, const char *ptr, size_t len) {
    vqView v = alloc_view(&mapvtab, empty_meta(env), 0, sizeof(struct vqMap_s));
    vqMap map = MappedFile(v);
    
    map->data = ptr;
    map->size = len;
    map->vref = vq_incref(parent);

    if (t != 0) {
        lua_pushvalue(env, t);
        map->parent = luaL_ref(env, LUA_REGISTRYINDEX);
    } else
        map->parent = LUA_NOREF;

    lua_newtable(env);
    map->areas = luaL_ref(env, LUA_REGISTRYINDEX);

    return v;
}

static int push_map (vqView map) {
    vqView *ud = tagged_udata(vwEnv(map), sizeof *ud, "vq.map");
    *ud = vq_incref(map);
    return 1;
}

static vqView checkmap (vqEnv env, int t) {
    if (lua_isstring(env, t)) {
        size_t n;
        const char *s = lua_tolstring(env, t, &n);
        return new_map(env, t, 0, s, n);
    }
    return *(vqView*) luaL_checkudata(env, t, "vq.map");
}

static void *map_lookup (vqView v, const char *base) {
    void *result;
    vqEnv env = vwEnv(v);
    lua_getref(env, MappedFile(v)->areas);
    lua_pushlightuserdata(env, (char*) base);
    lua_rawget(env, -2);
    result = lua_touserdata(env, -1);
    lua_pop(env, 1);
    return result;
}

static void map_remember (vqView v, const char *base, void *ptr) {
    vqEnv env = vwEnv(v);
    lua_getref(env, MappedFile(v)->areas);
    lua_pushlightuserdata(env, (char*) base);
    if (ptr != 0)
        lua_pushlightuserdata(env, ptr);
    else
        lua_pushnil(env);
    lua_rawset(env, -3);
    lua_pop(env, 1);
}

static int map_gc (vqEnv env) {
    vqView map = checkmap(env, 1);
    vq_decref(map);
    return 0;
}
static int map_index (vqEnv env) {
    vqView map = checkmap(env, 1);
    int pos = luaL_checkinteger(env, 2);
    if (pos < 1 || (size_t) pos > MappedFile(map)->size)
        return 0;
    lua_pushinteger(env, MappedFile(map)->data[pos-1] & 0xFF);
    return 1;
}
static int map_len (vqEnv env) {
    vqView map = checkmap(env, 1);
    lua_pushinteger(env, MappedFile(map)->size);
    return 1;
}
static int map2string (vqEnv env) {
    vqView map = checkmap(env, 1);
    lua_pushlstring(env, MappedFile(map)->data, MappedFile(map)->size);
    return 1;
}

static const struct luaL_reg vqlib_map_m[] = {
    {"__gc", map_gc},
    {"__index", map_index},
    {"__len", map_len},
    {"__tostring", map2string},
    {0, 0},
};

/* create a map from a filename or a string (with given offset and length) */
static int vq_map (vqEnv env) {
    vqView map = 0;
    size_t n;
    const char *s = lua_tolstring(env, 1, &n);
    if (s == 0) {
        vqView parent = checkmap(env, 1);
        if (parent != 0)
            map = new_map(env, 0, parent, MappedFile(parent)->data,
                                            MappedFile(parent)->size);
    } else if (lua_isnumber(env, 2)) {
        map = new_map(env, 1, 0, s, n);
    } else {
        s = OpenMappedFile(s, &n);
        if (s != 0)
            map = new_map(env, 0, 0, s, n);
    }
    if (map == 0)
        return 0;
    if (lua_isnumber(env, 2)) {
        int o = check_pos(env, 2);
        MappedFile(map)->data += o;
        MappedFile(map)->size = luaL_optint(env, 3, MappedFile(map)->size - o);
    }
    return push_map(map);
}
/* load a view from a memory-mapped file or from a string */
static int vq_load (vqEnv env) {
    vqView v = LoadView(checkmap(env, -1));
    if (v == 0) {
        lua_pushnil(env);
        lua_pushstring(env, "data is not a view");
        return 2;
    }
    return push_view(v);    
}

static const struct luaL_reg vqlib_map_f[] = {
    {"load", vq_load},
    {"map", vq_map},
    {0, 0},
};
