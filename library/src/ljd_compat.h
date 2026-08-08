#ifndef LJD_COMPAT_H
#define LJD_COMPAT_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <stdexcept>

/* Portable replacements for the Windows API used by the core decompiler.
 * This header is included ONLY for library/Android builds.
 */

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

using HANDLE = FILE*;
#define INVALID_HANDLE_VALUE ((FILE*)0)

/* ------------------------------------------------------------------ */
/* Minimal file helpers backed by stdio                                */
/* ------------------------------------------------------------------ */
static inline FILE* ljd_fopen_read(const char* path) {
    return path && *path ? fopen(path, "rb") : nullptr;
}
static inline FILE* ljd_fopen_write(const char* path) {
    return path && *path ? fopen(path, "wb") : nullptr;
}
static inline void ljd_close_file(FILE* h) {
    if (h) fclose(h);
}
static inline uint64_t ljd_get_file_size(FILE* h) {
    if (!h) return 0;
    long cur = ftell(h);
    fseek(h, 0, SEEK_END);
    long size = ftell(h);
    fseek(h, cur, SEEK_SET);
    return (uint64_t)(size > 0 ? size : 0);
}
static inline bool ljd_read_file(FILE* h, uint8_t* buf, uint32_t len, uint32_t* outRead) {
    if (!h || !buf || !outRead) return false;
    size_t rd = fread(buf, 1, len, h);
    *outRead = (uint32_t)rd;
    return rd == (size_t)len;
}
static inline bool ljd_write_file(FILE* h, const void* buf, uint32_t len, uint32_t* outWritten) {
    if (!h || !buf || !outWritten) return false;
    size_t wr = fwrite(buf, 1, len, h);
    *outWritten = (uint32_t)wr;
    return wr == (size_t)len;
}

/* Map WinAPI names used in the core code to portable equivalents */
#define CreateFileA      ljd_fopen_write
#define ReadFile(h,b,l,rd,ov) ljd_read_file((h),(b),(l),(rd))
#define WriteFile(h,b,l,rd,ov) ljd_write_file((h),(b),(l),(rd))
#define CloseHandle      ljd_close_file
#define GetFileSize      ljd_get_file_size
#define INVALID_HANDLE_VALUE ((FILE*)0)
#define GENERIC_READ     0
#define GENERIC_WRITE    0
#define OPEN_EXISTING    0
#define CREATE_ALWAYS    0
#define FILE_ATTRIBUTE_NORMAL 0
#define FILE_FLAG_SEQUENTIAL_SCAN 0
#define DWORD            uint32_t

/* Error handling used in the Windows build */
#define MessageBoxA(...)
#define MB_ICONWARNING  0
#define MB_YESNO        0
#define MB_DEFBUTTON2   0
#define MB_ICONERROR    0
#define MB_OK           0
#define IDYES           1
#define IDCANCEL        2
#define IDTRYAGAIN      10
#define IDCONTINUE      11

/* ------------------------------------------------------------------ */
/* C++17 polyfill for std::bit_cast (requires C++20)                 */
/* ------------------------------------------------------------------ */
namespace ljd {
template <typename To, typename From>
static inline To bit_cast(const From& f) {
    static_assert(sizeof(To) == sizeof(From), "bit_cast size mismatch");
    To t;
    memcpy(&t, &f, sizeof(t));
    return t;
}
}

#endif /* LJD_COMPAT_H */
