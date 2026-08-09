#include <jni.h>
#include <string>
#include <vector>
#include <cstring>

#include "ljd/ljd.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_marsinator_ljd_Ljd_strerror(JNIEnv* env, jclass /*cls*/, jint code) {
    const char* msg = ljd_strerror((int)code);
    if (!msg) msg = "Unknown error";
    return env->NewStringUTF(msg);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_marsinator_ljd_Ljd_version(JNIEnv* env, jclass /*cls*/) {
    return env->NewStringUTF(ljd_version());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_marsinator_ljd_Ljd_cacheDir(JNIEnv* env, jclass /*cls*/) {
    const char* d = ljd_cache_dir(nullptr);
    if (!d || !*d) return env->NewStringUTF("");
    return env->NewStringUTF(d);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_marsinator_ljd_Ljd_decompile(
        JNIEnv* env,
        jclass /*cls*/,
        jbyteArray bytecode,
        jint options,
        jstring cacheDir) {

    if (!bytecode) {
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, ljd_strerror(LJD_ERR_NULL_PTR));
        return nullptr;
    }

    jsize len = env->GetArrayLength(bytecode);
    if (len <= 0) {
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, ljd_strerror(LJD_ERR_EMPTY_IN));
        return nullptr;
    }

    std::vector<uint8_t> buf((size_t)len);
    env->GetByteArrayRegion(bytecode, 0, len, reinterpret_cast<jbyte*>(buf.data()));

    /* Propagate the app's files dir into the decompiler so that all
     * temporary files land there instead of /tmp. */
    if (cacheDir) {
        const char* c = env->GetStringUTFChars(cacheDir, nullptr);
        if (c && *c) setenv("LJD_CACHE_DIR", c, 1);
        env->ReleaseStringUTFChars(cacheDir, c);
    }

    ljd_ctx* ctx = ljd_init((uint32_t)options);
    if (!ctx) {
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, ljd_strerror(LJD_ERR_OOM));
        return nullptr;
    }

    char* out = nullptr;
    size_t out_sz = 0;
    int rc = ljd_decompile(ctx, buf.data(), buf.size(), &out, &out_sz);
    ljd_close(ctx);

    if (rc != LJD_OK) {
        if (out) ljd_free_string(out);
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, ljd_strerror(rc));
        return nullptr;
    }

    if (!out) {
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, "Decompiler returned empty output");
        return nullptr;
    }

    jstring result = env->NewStringUTF(out);
    ljd_free_string(out);
    return result;

extern "C" JNIEXPORT jstring JNICALL
Java_com_marsinator_ljd_Ljd_decompileFile(
        JNIEnv* env,
        jclass /*cls*/,
        jstring inputPath,
        jstring outputPath,
        jboolean overwrite) {

    if (!inputPath || !outputPath) {
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, ljd_strerror(LJD_ERR_NULL_PTR));
        return nullptr;
    }

    const char* in_c = env->GetStringUTFChars(inputPath, nullptr);
    const char* out_c = env->GetStringUTFChars(outputPath, nullptr);
    if (!in_c || !out_c) {
        if (in_c) env->ReleaseStringUTFChars(inputPath, in_c);
        if (out_c) env->ReleaseStringUTFChars(outputPath, out_c);
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, ljd_strerror(LJD_ERR_NULL_PTR));
        return nullptr;
    }

    ljd_ctx* ctx = ljd_init(0);
    if (!ctx) {
        env->ReleaseStringUTFChars(inputPath, in_c);
        env->ReleaseStringUTFChars(outputPath, out_c);
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, ljd_strerror(LJD_ERR_OOM));
        return nullptr;
    }

    char* out = nullptr;
    size_t out_sz = 0;
    int rc = ljd_decompile_file(ctx, in_c, out_c, overwrite, &out, &out_sz);
    ljd_close(ctx);

    env->ReleaseStringUTFChars(inputPath, in_c);
    env->ReleaseStringUTFChars(outputPath, out_c);

    if (rc != LJD_OK) {
        if (out) ljd_free_string(out);
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, ljd_strerror(rc));
        return nullptr;
    }

    if (!out) {
        jclass exc = env->FindClass("com/marsinator/ljd/LjdException");
        env->ThrowNew(exc, "Decompiler returned empty output");
        return nullptr;
    }

    jstring result = env->NewStringUTF(out);
    ljd_free_string(out);
    return result;
}

}
