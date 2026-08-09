#include "ljd/ljd.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

#include "ljd_compat.h"



using namespace std;

class Bytecode;
class Ast;
class Lua;

#include "bytecode/bytecode.h"
#include "ast/ast.h"
#include "lua/lua.h"

/* ------------------------------------------------------------------ */
/* Library-local no-op implementations for the UI helpers that the    */
/* original Windows desktop binary expected.  They are exported here  */
/* so that the core modules (bytecode/ast/lua) can call them without */
/* pulling in Windows headers.                                        */
/* ------------------------------------------------------------------ */
void print(const std::string& message) {
    (void)message;
}

void print_progress_bar(const double& /*progress*/, const double& /*total*/) {
}

void erase_progress_bar() {
}

void assert(const bool& assertion,
            const std::string& message,
            const std::string& /*filePath*/,
            const std::string& /*function*/,
            const std::string& /*source*/,
            const uint32_t& /*line*/) {
    if (!assertion) {
        throw runtime_error(message);
    }
}

std::string byte_to_string(const uint8_t& byte) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02X", byte);
    return string(buf);
}

/* ------------------------------------------------------------------ */
/* Portable cache-directory helpers                                    */
/* ------------------------------------------------------------------ */
static string default_cache_dir() {
#if defined(__ANDROID__) && defined(LJD_ANDROID)
    const char* env = getenv("LJD_CACHE_DIR");
    if (env && *env) return string(env);
#endif
    const char* tmp = getenv("TMPDIR");
    if (tmp && *tmp) return string(tmp);
#ifdef _WIN32
    char buf[MAX_PATH];
    GetTempPathA(MAX_PATH, buf);
    return string(buf);
#else
    return string("/tmp");
#endif
}

static string make_temp_path(const string& cacheDir, const string& tag) {
    char buf[512];
    snprintf(buf, sizeof(buf), "%s/ljd_%s_XXXXXX", cacheDir.c_str(), tag.c_str());
    return string(buf);
}

namespace ljd {

struct Decompiler {
    uint32_t options;
    string cacheDir;

    explicit Decompiler(uint32_t opts)
        : options(opts)
        , cacheDir(default_cache_dir())
    {}

    int run(const uint8_t* data, size_t size, string* out) {
        if (!data || !size || !out) return LJD_ERR_NULL_PTR;

        if (size < 7) return LJD_ERR_EMPTY_IN;

        if (data[0] != 0x1B || data[1] != 'L' || data[2] != 'J')
            return LJD_ERR_BAD_MAGIC;

        if (data[3] != 1 && data[3] != 2)
            return LJD_ERR_BAD_VER;

        try {
            char inPath[512];
            snprintf(inPath, sizeof(inPath),
                     "%s/ljd_in_%p.bin", cacheDir.c_str(), (void*)out);

            {
                FILE* f = fopen(inPath, "wb");
                if (!f) return LJD_ERR_READ;
                fwrite(data, 1, size, f);
                fclose(f);
            }

            Bytecode bc{string(inPath)};
            bc();

            Ast ast(bc, (options & LJD_OPT_IGNORE_DEBUG_INFO) != 0,
                    (options & LJD_OPT_MINIMIZE_DIFFS) != 0);
            ast();

            char outPath[512];
            snprintf(outPath, sizeof(outPath),
                     "%s/ljd_out_%p.lua", cacheDir.c_str(), (void*)out);

            Lua lua(bc, ast, string(outPath), true,
                    (options & LJD_OPT_MINIMIZE_DIFFS) != 0,
                    (options & LJD_OPT_UNRESTRICTED_ASCII) != 0);
            lua();

            {
                FILE* f = fopen(outPath, "rb");
                if (!f) {
                    std::remove(inPath);
                    return LJD_ERR_READ;
                }
                fseek(f, 0, SEEK_END);
                long len = ftell(f);
                fseek(f, 0, SEEK_SET);
                if (len < 0) {
                    fclose(f);
                    std::remove(inPath);
                    std::remove(outPath);
                    return LJD_ERR_READ;
                }
                out->resize((size_t)len);
                if (len > 0) {
                    size_t rd = fread(out->data(), 1, (size_t)len, f);
                    if ((long)rd != len) {
                        fclose(f);
                        std::remove(inPath);
                        std::remove(outPath);
                        return LJD_ERR_READ;
                    }
                }
                fclose(f);
                std::remove(outPath);
            }

            std::remove(inPath);
            return LJD_OK;
        } catch (const std::bad_alloc&) {
            return LJD_ERR_OOM;
        } catch (...) {
            return LJD_ERR_INTERNAL;
        }
    }
};

} // namespace ljd

extern "C" ljd_ctx* ljd_init(uint32_t options) {
    return reinterpret_cast<ljd_ctx*>(new (nothrow) ljd::Decompiler(options));
}

extern "C" int ljd_decompile(ljd_ctx* ctx,
                              const uint8_t* bytecode,
                              size_t bytecode_sz,
                              char** out,
                              size_t* out_sz) {
    *out = nullptr;
    *out_sz = 0;
    if (!ctx || !bytecode || !bytecode_sz || !out) return LJD_ERR_NULL_PTR;

    string result;
    int rc = reinterpret_cast<ljd::Decompiler*>(ctx)->run(bytecode, bytecode_sz, &result);
    if (rc == LJD_OK) {
        *out = (char*)malloc(result.size() + 1);
        if (!*out) return LJD_ERR_OOM;
        memcpy(*out, result.data(), result.size());
        (*out)[result.size()] = '\0';
        *out_sz = result.size();
    }
    return rc;
}

extern "C" void ljd_free_string(char* str) {
    free(str);
}

extern "C" void ljd_close(ljd_ctx* ctx) {
    delete reinterpret_cast<ljd::Decompiler*>(ctx);
}

extern "C" const char* ljd_strerror(int code) {
    switch (code) {
        case LJD_OK:              return "OK";
        case LJD_ERR_NULL_PTR:    return "Null pointer argument";
        case LJD_ERR_EMPTY_IN:    return "Empty input buffer";
        case LJD_ERR_BAD_MAGIC:   return "Invalid LuaJIT bytecode magic";
        case LJD_ERR_BAD_VER:     return "Unsupported bytecode version";
        case LJD_ERR_READ:        return "Read/write error";
        case LJD_ERR_PARSE:       return "Decompilation failed";
        case LJD_ERR_INTERNAL:    return "Internal error";
        case LJD_ERR_OOM:         return "Out of memory";
        default:                  return "Unknown error";
    }
}

extern "C" const char* ljd_version(void) {
    return "luajit-decompiler-v2 1.0.0";
}

extern "C" const char* ljd_cache_dir(const ljd_ctx* ctx) {
    if (!ctx) return "";
    return reinterpret_cast<const ljd::Decompiler*>(ctx)->cacheDir.c_str();
}

extern "C" int ljd_decompile_file(ljd_ctx* ctx,
                                  const char* input_path,
                                  const char* output_path,
                                  bool overwrite,
                                  char** out,
                                  size_t* out_sz) {
    *out = nullptr;
    *out_sz = 0;
    if (!ctx || !input_path || !output_path || !out) return LJD_ERR_NULL_PTR;

    ljd::Decompiler* decomp = reinterpret_cast<ljd::Decompiler*>(ctx);
    uint32_t options = decomp->options;

    try {
        /* Check overwrite flag */
        if (!overwrite) {
            FILE* f = fopen(output_path, "rb");
            if (f) {
                fclose(f);
                return LJD_ERR_READ;
            }
        }

        Bytecode bc{string(input_path)};
        bc();

        Ast ast(bc, (options & LJD_OPT_IGNORE_DEBUG_INFO) != 0,
                (options & LJD_OPT_MINIMIZE_DIFFS) != 0);
        ast();

        Lua lua(bc, ast, string(output_path), overwrite,
                (options & LJD_OPT_MINIMIZE_DIFFS) != 0,
                (options & LJD_OPT_UNRESTRICTED_ASCII) != 0);
        lua();

        /* Read back the written file to return as string */
        {
            FILE* f = fopen(output_path, "rb");
            if (!f) return LJD_ERR_READ;
            fseek(f, 0, SEEK_END);
            long len = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (len < 0) {
                fclose(f);
                return LJD_ERR_READ;
            }
            string result;
            result.resize((size_t)len);
            if (len > 0) {
                size_t rd = fread(&result[0], 1, (size_t)len, f);
                if ((long)rd != len) {
                    fclose(f);
                    return LJD_ERR_READ;
                }
            }
            fclose(f);
            *out = (char*)malloc(result.size() + 1);
            if (!*out) return LJD_ERR_OOM;
            memcpy(*out, result.data(), result.size());
            (*out)[result.size()] = '\0';
            *out_sz = result.size();
        }

        return LJD_OK;
    } catch (const std::bad_alloc&) {
        return LJD_ERR_OOM;
    } catch (...) {
        return LJD_ERR_INTERNAL;
    }
}

