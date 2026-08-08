package com.marsinator.ljd;

public final class Ljd {
    static {
        System.loadLibrary("ljd");
    }

    public static final int OPT_NONE = 0;
    public static final int OPT_IGNORE_DEBUG_INFO = 1;
    public static final int OPT_MINIMIZE_DIFFS = 2;
    public static final int OPT_UNRESTRICTED_ASCII = 4;

    private Ljd() {}

    /**
     * Decompile LuaJIT bytecode buffer to Lua source.
     *
     * @param bytecode bytecode bytes
     * @param options bitmask of {@code Ljd.OPT_*}
     * @return decompiled Lua source as UTF-8 string
     * @throws LjdException if decompilation fails
     */
    public static native String decompile(byte[] bytecode, int options) throws LjdException;

    /**
     * Get description of an error code returned by the native layer.
     */
    public static native String strerror(int code);
}
