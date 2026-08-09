#ifndef LJD_H
#define LJD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes returned by ljd_decompile. */
#define LJD_OK            0
#define LJD_ERR_NULL_PTR  1
#define LJD_ERR_EMPTY_IN  2
#define LJD_ERR_BAD_MAGIC 3
#define LJD_ERR_BAD_VER   4
#define LJD_ERR_READ      5
#define LJD_ERR_PARSE     6
#define LJD_ERR_INTERNAL  7
#define LJD_ERR_OOM       8

/* Options bitmask passed to ljd_decompile. */
#define LJD_OPT_NONE                0
#define LJD_OPT_IGNORE_DEBUG_INFO   1
#define LJD_OPT_MINIMIZE_DIFFS      2
#define LJD_OPT_UNRESTRICTED_ASCII  4

/*
 * Opaque handle to an internal decompilation context.
 *
 * The caller must not read or modify its contents.
 */
struct ljd_ctx { void* p; };

/*
 * Initialise the decompiler library. Returns NULL on failure.
 *
 * This function is thread-safe as long as it is not called concurrently
 * with ljd_decompile using the same options pointer (it does not allocate
 * any global state).
 */
ljd_ctx* ljd_init(uint32_t options);

/*
 * Decompile a LuaJIT bytecode buffer.
 *
 *   ctx        - initialised context returned by ljd_init.
 *   bytecode   - pointer to the bytecode bytes.
 *   bytecode_sz- size of the bytecode buffer in bytes.
 *   out        - receives a NUL-terminated UTF-8 string on success.
 *                The caller must free this buffer with ljd_free_string.
 *   out_sz     - receives the byte length of the output string, excluding
 *                the trailing NUL.
 *
 * Returns LJD_OK on success, or one of the LJD_ERR_* codes on failure.
 * On failure, *out and *out_sz are set to NULL and 0 respectively.
 */
int ljd_decompile(ljd_ctx* ctx,
                  const uint8_t* bytecode,
                  size_t bytecode_sz,
                  char** out,
                  size_t* out_sz);

/*
 * Free a string previously returned in *out by ljd_decompile.
 *
 * It is safe to pass NULL.
 */
void ljd_free_string(char* str);

/*
 * Release any resources held by the context.
 *
 * It is safe to pass NULL.
 */
void ljd_close(ljd_ctx* ctx);

/*
 * Return a human-readable description of an error code.
 *
 * The returned pointer points to a static string and must not be freed.
 */
const char* ljd_strerror(int code);

/*
 * Query the version string of the decompiler library.
 *
 * The returned pointer points to a static string and must not be freed.
 */
const char* ljd_version(void);

/*
 * Return the recommended cache directory for the decompiler.
 *
 * On Android this returns the app-specific files directory passed at
 * init time. On other platforms it returns a default temporary path.
 *
 * The caller must not free the returned string.
 */
const char* ljd_cache_dir(const ljd_ctx* ctx);


/*
 * Decompile a LuaJIT bytecode file to a source file.
 *
 *   ctx        - initialised context returned by ljd_init.
 *   input_path - path to the LuaJIT bytecode file.
 *   output_path- path where the decompiled source will be written.
 *   overwrite  - if false and output_path already exists, return LJD_ERR_READ.
 *   out        - receives a NUL-terminated UTF-8 string on success.
 *   out_sz     - receives the byte length of the output string, excluding
 *                the trailing NUL.
 *
 * Returns LJD_OK on success, or one of the LJD_ERR_* codes on failure.
 * On failure, *out and *out_sz are set to NULL and 0 respectively.
 */
int ljd_decompile_file(ljd_ctx* ctx,
                       const char* input_path,
                       const char* output_path,
                       bool overwrite,
                       char** out,
                       size_t* out_sz);

#ifdef __cplusplus
}
#endif

#endif /* LJD_H */
