# libLJD

`libLJD` is a portable, Android-focused shared library (`libljd.so`) that wraps the
[luajit-decompiler-v2](https://github.com/marsinator358/luajit-decompiler-v2) behind a
small C API, with a JNI bridge for Java/Kotlin consumers. The library reads LuaJIT
bytecode (version 1 and 2), reconstructs the source and returns it as a UTF-8 string.

Target: **Android aarch64** (arm64-v8a). The same C API works from any language that can
call C functions or load a `.so`.

## Building

### Prerequisites

- CMake 3.22+
- Android NDK r27+ (only for Android cross-compilation; a local Linux build is provided
  for development without the NDK).

### Local Linux build (development)

```bash
git clone https://github.com/4ndrxm3d4/libLJD.git
cd libLJD
cmake -S . -B build -DLJD_BUILD_TESTS=OFF
cmake --build build -j
```

The output shared library is:

```
build/libljd.so
```

### Android NDK cross-compilation

The GitHub Actions workflow `.github/workflows/android.yml` cross-compiles for
`aarch64-linux-android`. To build locally with the NDK:

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-21 \
  -DLJD_BUILD_TESTS=OFF
cmake --build build -j
```

## C API

The public header is `library/include/ljd/ljd.h`. It is intentionally small:

```c
#include <ljd/ljd.h>

/* Opaque handle. Call ljd_close() when done. */
ljd_ctx* ctx = ljd_init(LJD_OPT_NONE);
if (!ctx) { /* handle OOM */ }

char* out = NULL;
size_t out_sz = 0;
int rc = ljd_decompile(ctx, bytecode, bytecode_sz, &out, &out_sz);

if (rc == LJD_OK) {
    printf("%.*s\n", (int)out_sz, out);
    ljd_free_string(out);
} else {
    fprintf(stderr, "error: %s\n", ljd_strerror(rc));
}

ljd_close(ctx);
```

### Reading files from storage (C)

```c
#include <stdio.h>
#include "ljd/ljd.h"

int main() {
    FILE* f = fopen("/data/local/tmp/script.luac", "rb");
    if (!f) { perror("open"); return 1; }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* buf = malloc(sz);
    fread(buf, 1, sz, f);
    fclose(f);

    ljd_ctx* ctx = ljd_init(LJD_OPT_NONE);
    char* out = NULL;
    size_t out_sz = 0;
    int rc = ljd_decompile(ctx, buf, sz, &out, &out_sz);
    ljd_close(ctx);
    free(buf);

    if (rc == LJD_OK) {
        printf("%s", out);
        ljd_free_string(out);
    }
    return 0;
}
```

### File-based decompilation

The same library also exposes a file-to-file API:

```c
ljd_ctx* ctx = ljd_init(LJD_OPT_NONE);
char* out = NULL;
size_t out_sz = 0;
int rc = ljd_decompile_file(ctx, "input.luac", "output.lua", false, &out, &out_sz);

if (rc == LJD_OK) {
    printf("Decompiled %zu bytes\n", out_sz);
    ljd_free_string(out);
} else {
    fprintf(stderr, "error: %s\n", ljd_strerror(rc));
}

ljd_close(ctx);
```

### Error codes

| Code | Meaning |
|------|---------|
| `LJD_OK` | Success. |
| `LJD_ERR_NULL_PTR` | A required argument was NULL. |
| `LJD_ERR_EMPTY_IN` | The bytecode buffer is empty. |
| `LJD_ERR_BAD_MAGIC` | The first bytes are not `0x1B 0x4C 0x4A`. |
| `LJD_ERR_BAD_VER` | Unsupported LuaJIT bytecode version. |
| `LJD_ERR_READ` | File or memory read error during decompilation. |
| `LJD_ERR_PARSE` | The decompiler failed to reconstruct source. |
| `LJD_ERR_INTERNAL` | Unexpected exception. |
| `LJD_ERR_OOM` | Out of memory. |

### Options

Pass a bitmask to `ljd_init()`:

| Flag | Effect |
|------|--------|
| `LJD_OPT_NONE` | Default behaviour. |
| `LJD_OPT_IGNORE_DEBUG_INFO` | Strip debug information from the output. |
| `LJD_OPT_MINIMIZE_DIFFS` | Prefer formatting that produces smaller diffs. |
| `LJD_OPT_UNRESTRICTED_ASCII` | Allow non-printable ASCII in identifiers. |

### Thread safety

`ljd_init()` and `ljd_decompile()` are thread-safe as long as each thread uses its own
`ljd_ctx`. The library does not keep global mutable state.

## Using from other languages

All examples below assume `libljd.so` is on the system/library path or in the current
directory.

### C / C++

```c
#include <ljd/ljd.h>

int main(void) {
    ljd_ctx* ctx = ljd_init(LJD_OPT_NONE);
    /* read bytecode into `buf` and `len` ... */
    char* out = NULL;
    size_t out_sz = 0;
    int rc = ljd_decompile(ctx, buf, len, &out, &out_sz);
    if (rc == LJD_OK) {
        fwrite(out, 1, out_sz, stdout);
        ljd_free_string(out);
    }
    ljd_close(ctx);
    return rc;
}
```

Compile:

```bash
gcc -o example example.c -I./library/include -L./build -lljd -Wl,-rpath,./build
```

### Java (JNI)

The library exposes JNI functions in the package `com.marsinator.ljd.Ljd`.

```java
package com.marsinator.ljd;

public class Ljd {
    static { System.loadLibrary("ljd"); }

    public static native String strerror(int code);
    public static native String version();
    public static native String cacheDir();
    public static native String decompile(byte[] bytecode, int options, String cacheDir);
    public static native String decompileFile(String inputPath, String outputPath, boolean overwrite);
}
```

Usage:

```java
// In-memory decompilation
byte[] bytecode = Files.readAllBytes(path);
String source = Ljd.decompile(bytecode, LjdOptions.NONE, context.getCacheDir().getAbsolutePath());
System.out.println(source);

// File-based decompilation
String result = Ljd.decompileFile("/data/local/tmp/script.luac", "/data/local/tmp/script.lua", false);
System.out.println(result);
```

On Android, add the prebuilt `libljd.so` to `src/main/jniLibs/arm64-v8a/`.

### Kotlin

```kotlin
import com.marsinator.ljd.Ljd

object LjdOptions {
    const val NONE = 0
    const val IGNORE_DEBUG_INFO = 1
    const val MINIMIZE_DIFFS = 2
    const val UNRESTRICTED_ASCII = 4
}

fun decompile(bytecode: ByteArray, cacheDir: File): String? {
    return Ljd.decompile(bytecode, LjdOptions.NONE, cacheDir.absolutePath)
}

fun decompileFile(inputPath: String, outputPath: String, overwrite: Boolean = false): String? {
    return Ljd.decompileFile(inputPath, outputPath, overwrite)
}
```

### Python (ctypes)

```python
import ctypes
from pathlib import Path

lib = ctypes.CDLL(str(Path("build/libljd.so").resolve()))
lib.ljd_init.restype = ctypes.c_void_p
lib.ljd_decompile_file.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_bool, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_size_t)]
lib.ljd_decompile_file.restype = ctypes.c_int

# Reading bytecode from file
bytecode = Path("/data/local/tmp/script.luac").read_bytes()
in_buf = ctypes.create_string_buffer(bytecode, len(bytecode))

ctx = lib.ljd_init(0)
out = ctypes.c_char_p()
out_sz = ctypes.c_size_t()

# In-memory decompile
rc = lib.ljd_decompile(ctx, in_buf, len(bytecode), ctypes.byref(out), ctypes.byref(out_sz))

# Or file-to-file
# rc = lib.ljd_decompile_file(ctx, b"/data/local/tmp/script.luac", b"/data/local/tmp/out.lua", False, ctypes.byref(out), ctypes.byref(out_sz))

lib.ljd_close(ctx)
if rc == 0 and out.value:
    print(out.value.decode())
    lib.ljd_free_string(out)
```
from pathlib import Path

lib = ctypes.CDLL(str(Path("build/libljd.so").resolve()))

class Ctx(ctypes.c_void_p):
    pass

lib.ljd_init.restype = Ctx
lib.ljd_init.argtypes = [ctypes.c_uint32]

lib.ljd_decompile.restype = ctypes.c_int
lib.ljd_decompile.argtypes = [
    Ctx,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_char_p),
    ctypes.POINTER(ctypes.c_size_t),
]

lib.ljd_free_string.restype = None
lib.ljd_free_string.argtypes = [ctypes.c_char_p]

lib.ljd_close.restype = None
lib.ljd_close.argtypes = [Ctx]

ctx = lib.ljd_init(0)
buf = (ctypes.c_uint8 * len(bytecode))(*bytecode)
out = ctypes.c_char_p()
out_sz = ctypes.c_size_t()
rc = lib.ljd_decompile(ctx, buf, len(bytecode), ctypes.byref(out), ctypes.byref(out_sz))
if rc == 0:
    print(out.value[:out_sz.value])
    lib.ljd_free_string(out)
lib.ljd_close(ctx)
```

### Rust (ctypes)

```rust
use std::ffi::{c_char, c_size_t, c_uint, c_uint8};
use std::os::raw::c_void;

#[repr(C)]
struct Ctx(c_void);

extern "C" {
    fn ljd_init(opts: c_uint) -> *mut Ctx;
    fn ljd_decompile(
        ctx: *mut Ctx,
        bytecode: *const c_uint8,
        bytecode_sz: c_size_t,
        out: *mut *mut c_char,
        out_sz: *mut c_size_t,
    ) -> c_int;
    fn ljd_free_string(s: *mut c_char);
    fn ljd_close(ctx: *mut Ctx);
}

fn decompile(bytecode: &[u8]) -> Option<String> {
    let ctx = unsafe { ljd_init(0) };
    if ctx.is_null() { return None; }
    let mut out = std::ptr::null_mut();
    let mut out_sz = 0;
    let rc = unsafe {
        ljd_decompile(
            ctx,
            bytecode.as_ptr(),
            bytecode.len(),
            &mut out,
            &mut out_sz,
        )
    };
    if rc != 0 {
        unsafe { ljd_close(ctx) };
        return None;
    }
    let result = unsafe { String::from_raw_parts(out as *mut u8, out_sz, out_sz) };
    unsafe { ljd_free_string(out) };
    unsafe { ljd_close(ctx) };
    Some(result)
}
```

### Swift / Objective-C

```objc
#import <Foundation/Foundation.h>

// Declare the C functions.
ljd_ctx* ljd_init(uint32_t options);
int ljd_decompile(ljd_ctx* ctx, const uint8_t* bytecode, size_t bytecode_sz,
                  char** out, size_t* out_sz);
void ljd_free_string(char* str);
void ljd_close(ljd_ctx* ctx);
const char* ljd_strerror(int code);

NSString* LjdDecompile(NSData* bytecode) {
    ljd_ctx* ctx = ljd_init(0);
    if (!ctx) return nil;

    char* out = NULL;
    size_t out_sz = 0;
    int rc = ljd_decompile(ctx, bytecode.bytes, bytecode.length, &out, &out_sz);

    if (rc == 0) {
        NSString* result = [[NSString alloc] initWithBytes:out length:out_sz encoding:NSUTF8StringEncoding];
        ljd_free_string(out);
        ljd_close(ctx);
        return result;
    }

    ljd_close(ctx);
    return nil;
}
```

### C# / .NET (P/Invoke)

```csharp
using System;
using System.Runtime.InteropServices;

class Ljd {
    [DllImport("libljd.so", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr ljd_init(uint options);

    [DllImport("libljd.so", CallingConvention = CallingConvention.Cdecl)]
    public static extern int ljd_decompile(IntPtr ctx, byte[] bytecode, UIntPtr bytecode_sz,
                                           out IntPtr outp, out UIntPtr out_szp);

    [DllImport("libljd.so", CallingConvention = CallingConvention.Cdecl)]
    public static extern void ljd_free_string(IntPtr s);

    [DllImport("libljd.so", CallingConvention = CallingConvention.Cdecl)]
    public static extern void ljd_close(IntPtr ctx);
}

string? Decompile(byte[] bytecode) {
    var ctx = Ljd.ljd_init(0);
    if (ctx == IntPtr.Zero) return null;

    int rc = Ljd.ljd_decompile(ctx, bytecode, (UIntPtr)bytecode.Length, out var outp, out var out_sz);
    if (rc != 0) {
        Ljd.ljd_close(ctx);
        return null;
    }

    string result = Marshal.PtrToStringUTF8(outp, (int)out_sz);
    Ljd.ljd_free_string(outp);
    Ljd.ljd_close(ctx);
    return result;
}
```

## Cache directory

On Android the decompiler writes temporary files while working. By default it uses
`/tmp`, which is not writable for normal apps. Set the environment variable
`LJD_CACHE_DIR` to your app's files directory before calling `ljd_init()`:

```java
System.setProperty("LJD_CACHE_DIR", context.getFilesDir().getAbsolutePath());
```

In C:

```c
setenv("LJD_CACHE_DIR", "/data/data/com.example/app_files", 1);
ljd_init(0);
```

## Artifacts

Each successful CI run produces an `aarch64` build artifact containing:

```
libljd-aarch64/
  include/ljd/ljd.h   <- public C header
  lib/libLJD.so       <- the shared library
```

Download the artifact from the [GitHub Actions page](https://github.com/4ndrxm3d4/libLJD/actions).

## Repository layout

```
.
  ast/                     <- Lua AST reconstruction
  bytecode/                <- LuaJIT bytecode parser
  lua/                     <- Source code writer
  library/
    include/ljd/ljd.h       <- Public C API header
    src/
      ljd_api.cpp           <- C API implementation
      ljd_jni.cpp           <- JNI bridge (Android only)
  main.h / main.cpp         <- Original Windows desktop binary (not part of lib build)
  .github/workflows/
    android.yml              <- CI: Android aarch64 cross-compilation
```

## Contributing

Porting work happens in small, incremental commits. If you fix a build issue, please keep
the change focused and add a short commit message describing the failure mode and the fix.
