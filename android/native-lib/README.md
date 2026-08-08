# LuaJIT Decompiler Native - Android

This module packages `libljd.so` for use in Android applications.

## Usage

### 1. Load the library

```java
static {
    System.loadLibrary("ljd");
}
```

### 2. Call `Ljd.decompile(bytecode, options)`

```java
import com.marsinator.ljd.Ljd;

byte[] bytecode = Files.readAllBytes(Paths.get("/data/local/tmp/script.luac"));
String lua = Ljd.decompile(bytecode, Ljd.OPT_NONE);
System.out.println(lua);
```

### Options

| Flag | Value | Description |
|------|-------|-------------|
| `OPT_NONE` | 0 | Default |
| `OPT_IGNORE_DEBUG_INFO` | 1 | Strip debug info |
| `OPT_MINIMIZE_DIFFS` | 2 | Minimize formatting diffs |
| `OPT_UNRESTRICTED_ASCII` | 4 | Disable UTF-8 restrictions |

### Error handling

```java
try {
    String lua = Ljd.decompile(data, Ljd.OPT_NONE);
} catch (LjdException e) {
    Log.e("LJD", "Decompilation failed: " + e.getCode() + " -> " + e.getMessage());
}
```
