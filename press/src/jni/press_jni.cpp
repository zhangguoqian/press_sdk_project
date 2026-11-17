#include "jni.h"

#include "press/press.h"

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
constexpr const char* kPressSdkClassName = "PressSdk";
constexpr const char* kDataCallbacksClassName = "PressSdk$DataCallbacks";

struct JniCallbackContext
{
    JavaVM* vm = nullptr;
    jobject callbacks = nullptr;
    jclass callbacksClass = nullptr;
    jmethodID onReadOnlyDataMethod = nullptr;
    jmethodID onRealTimeDataMethod = nullptr;
    jmethodID onPressDataMethod = nullptr;
    jmethodID onTempDataMethod = nullptr;
    jmethodID onMixDataMethod = nullptr;
    jmethodID onErrorMethod = nullptr;
};

static std::mutex g_callbackMapMutex;
static std::unordered_map<CPressContext*, JniCallbackContext*> g_callbackMap;

static void clearPendingException(JNIEnv* env);

static jstring toJavaString(JNIEnv* env, const char* text)
{
    if (text == nullptr)
    {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(text);
}

static void clearPendingException(JNIEnv* env)
{
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

static JNIEnv* attachCurrentThread(JavaVM* vm, bool* didAttach)
{
    *didAttach = false;
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8) == JNI_EDETACHED)
    {
        if (vm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr) != JNI_OK)
        {
            return nullptr;
        }
        *didAttach = true;
    }
    return env;
}

static void detachCurrentThread(JavaVM* vm)
{
    vm->DetachCurrentThread();
}

static void jniOnReadOnlyData(void* userData,
                              int errorCode,
                              uint64_t registerNo,
                              const char* json)
{
    auto* ctx = static_cast<JniCallbackContext*>(userData);
    if (ctx == nullptr || ctx->vm == nullptr || ctx->callbacks == nullptr)
    {
        return;
    }

    bool didAttach = false;
    JNIEnv* env = attachCurrentThread(ctx->vm, &didAttach);
    if (env == nullptr)
    {
        return;
    }

    jstring javaJson = env->NewStringUTF(json == nullptr ? "" : json);

    if (ctx->onReadOnlyDataMethod != nullptr)
    {
        env->CallVoidMethod(ctx->callbacks,
                            ctx->onReadOnlyDataMethod,
                            errorCode,
                            static_cast<jlong>(registerNo),
                            javaJson);
    }

    if (javaJson != nullptr)
    {
        env->DeleteLocalRef(javaJson);
    }
    // Java 回调抛出的异常不能带入 Detach，否则 ART 会 abort
    clearPendingException(env);
    // 仅对本次 Attach 的线程 Detach；调用者自身（如 UI 线程）已附加时不能分离
    if (didAttach)
    {
        detachCurrentThread(ctx->vm);
    }
}

static void jniOnRealTimeData(void* userData,
                              int errorCode,
                              const char* json)
{
    auto* ctx = static_cast<JniCallbackContext*>(userData);
    if (ctx == nullptr || ctx->vm == nullptr || ctx->callbacks == nullptr)
    {
        return;
    }

    bool didAttach = false;
    JNIEnv* env = attachCurrentThread(ctx->vm, &didAttach);
    if (env == nullptr)
    {
        return;
    }

    jstring javaJson = env->NewStringUTF(json == nullptr ? "" : json);

    if (ctx->onRealTimeDataMethod != nullptr)
    {
        env->CallVoidMethod(ctx->callbacks,
                            ctx->onRealTimeDataMethod,
                            errorCode,
                            javaJson);
    }

    if (javaJson != nullptr)
    {
        env->DeleteLocalRef(javaJson);
    }
    clearPendingException(env);
    if (didAttach)
    {
        detachCurrentThread(ctx->vm);
    }
}

static void jniOnPressData(void* userData,
                           int errorCode,
                           const char* json)
{
    auto* ctx = static_cast<JniCallbackContext*>(userData);
    if (ctx == nullptr || ctx->vm == nullptr || ctx->callbacks == nullptr)
    {
        return;
    }

    bool didAttach = false;
    JNIEnv* env = attachCurrentThread(ctx->vm, &didAttach);
    if (env == nullptr)
    {
        return;
    }

    jstring javaJson = env->NewStringUTF(json == nullptr ? "" : json);

    if (ctx->onPressDataMethod != nullptr)
    {
        env->CallVoidMethod(ctx->callbacks,
                            ctx->onPressDataMethod,
                            errorCode,
                            javaJson);
    }

    if (javaJson != nullptr)
    {
        env->DeleteLocalRef(javaJson);
    }
    clearPendingException(env);
    if (didAttach)
    {
        detachCurrentThread(ctx->vm);
    }
}

static void jniOnTempData(void* userData,
                          int errorCode,
                          const char* json)
{
    auto* ctx = static_cast<JniCallbackContext*>(userData);
    if (ctx == nullptr || ctx->vm == nullptr || ctx->callbacks == nullptr)
    {
        return;
    }

    bool didAttach = false;
    JNIEnv* env = attachCurrentThread(ctx->vm, &didAttach);
    if (env == nullptr)
    {
        return;
    }

    jstring javaJson = env->NewStringUTF(json == nullptr ? "" : json);
    if (ctx->onTempDataMethod != nullptr)
    {
        env->CallVoidMethod(ctx->callbacks,
                            ctx->onTempDataMethod,
                            errorCode,
                            javaJson);
    }

    if (javaJson != nullptr)
    {
        env->DeleteLocalRef(javaJson);
    }
    clearPendingException(env);
    if (didAttach)
    {
        detachCurrentThread(ctx->vm);
    }
}

static void jniOnMixData(void* userData,
                         int errorCode,
                         const char* json)
{
    auto* ctx = static_cast<JniCallbackContext*>(userData);
    if (ctx == nullptr || ctx->vm == nullptr || ctx->callbacks == nullptr)
    {
        return;
    }

    bool didAttach = false;
    JNIEnv* env = attachCurrentThread(ctx->vm, &didAttach);
    if (env == nullptr)
    {
        return;
    }

    jstring javaJson = env->NewStringUTF(json == nullptr ? "" : json);
    if (ctx->onMixDataMethod != nullptr)
    {
        env->CallVoidMethod(ctx->callbacks,
                            ctx->onMixDataMethod,
                            errorCode,
                            javaJson);
    }

    if (javaJson != nullptr)
    {
        env->DeleteLocalRef(javaJson);
    }
    clearPendingException(env);
    if (didAttach)
    {
        detachCurrentThread(ctx->vm);
    }
}

static void jniOnError(void* userData,
                       uint16_t cmdCode,
                       const uint8_t* response,
                       size_t responseLen)
{
    auto* ctx = static_cast<JniCallbackContext*>(userData);
    if (ctx == nullptr || ctx->vm == nullptr || ctx->callbacks == nullptr)
    {
        return;
    }

    bool didAttach = false;
    JNIEnv* env = attachCurrentThread(ctx->vm, &didAttach);
    if (env == nullptr)
    {
        return;
    }

    jbyteArray responseArray = env->NewByteArray(static_cast<jsize>(responseLen));
    if (responseArray != nullptr && response != nullptr && responseLen > 0)
    {
        env->SetByteArrayRegion(responseArray,
                                0,
                                static_cast<jsize>(responseLen),
                                reinterpret_cast<const jbyte*>(response));
    }

    if (ctx->onErrorMethod != nullptr)
    {
        env->CallVoidMethod(ctx->callbacks,
                            ctx->onErrorMethod,
                            static_cast<jint>(cmdCode),
                            responseArray);
    }

    if (responseArray != nullptr)
    {
        env->DeleteLocalRef(responseArray);
    }
    clearPendingException(env);
    if (didAttach)
    {
        detachCurrentThread(ctx->vm);
    }
}

static void cleanupCallbackContext(JNIEnv* env, JniCallbackContext* ctx)
{
    if (ctx == nullptr)
    {
        return;
    }

    if (ctx->callbacks != nullptr)
    {
        env->DeleteGlobalRef(ctx->callbacks);
        ctx->callbacks = nullptr;
    }

    if (ctx->callbacksClass != nullptr)
    {
        env->DeleteGlobalRef(ctx->callbacksClass);
        ctx->callbacksClass = nullptr;
    }

    delete ctx;
}

} // namespace

extern "C" JNIEXPORT jlong JNICALL Java_PressSdk_press_create(JNIEnv* env, jclass)
{
    (void)env;
    CPressContext* handle = cpress_create();
    return reinterpret_cast<jlong>(handle);
}

extern "C" JNIEXPORT void JNICALL Java_PressSdk_press_destroy(JNIEnv* env, jclass, jlong handle)
{
    CPressContext* ctx = reinterpret_cast<CPressContext*>(handle);
    if (ctx == nullptr)
    {
        return;
    }

    // 先注销回调并停止 SDK 线程（join 完成后无在途回调），再清理 JNI 全局引用，
    // 避免定时线程回调正在执行时 jniCtx 被释放造成 use-after-free
    cpress_unregister_data_interface(ctx);

    {
        std::lock_guard<std::mutex> lock(g_callbackMapMutex);
        auto it = g_callbackMap.find(ctx);
        if (it != g_callbackMap.end())
        {
            cleanupCallbackContext(env, it->second);
            g_callbackMap.erase(it);
        }
    }

    cpress_destroy(ctx);
}

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_connect(JNIEnv* env, jclass, jlong handle, jstring portName, jint portType)
{
    (void)env;
    const char* value = env->GetStringUTFChars(portName, nullptr);
    if (value == nullptr)
    {
        return 0;
    }

    int result = cpress_connect(reinterpret_cast<CPressContext*>(handle), value, portType);
    env->ReleaseStringUTFChars(portName, value);
    return result;
}

extern "C" JNIEXPORT void JNICALL Java_PressSdk_press_disconnect(JNIEnv* env, jclass, jlong handle)
{
    (void)env;
    cpress_disconnect(reinterpret_cast<CPressContext*>(handle));
}

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_is_connected(JNIEnv* env, jclass, jlong handle)
{
    (void)env;
    return cpress_is_connected(reinterpret_cast<CPressContext*>(handle));
}

extern "C" JNIEXPORT jlong JNICALL Java_PressSdk_press_get_machine_register_no(JNIEnv* env, jclass, jlong handle)
{
    (void)env;
    return static_cast<jlong>(cpress_get_machine_register_no(reinterpret_cast<CPressContext*>(handle)));
}

extern "C" JNIEXPORT jstring JNICALL Java_PressSdk_press_get_last_error_info(JNIEnv* env, jclass, jlong handle)
{
    const char* value = cpress_get_last_error_info(reinterpret_cast<CPressContext*>(handle));
    return toJavaString(env, value);
}

extern "C" JNIEXPORT void JNICALL Java_PressSdk_press_run(JNIEnv* env, jclass, jlong handle)
{
    (void)env;
    cpress_run(reinterpret_cast<CPressContext*>(handle));
}

extern "C" JNIEXPORT void JNICALL Java_PressSdk_press_stop(JNIEnv* env, jclass, jlong handle)
{
    (void)env;
    cpress_stop(reinterpret_cast<CPressContext*>(handle));
}

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_is_running(JNIEnv* env, jclass, jlong handle)
{
    (void)env;
    return cpress_is_running(reinterpret_cast<CPressContext*>(handle));
}

extern "C" JNIEXPORT jstring JNICALL Java_PressSdk_press_get_read_only_data_json(JNIEnv* env, jclass, jlong handle, jint isCompressed)
{
    const char* json = cpress_get_read_only_data_json(reinterpret_cast<CPressContext*>(handle), isCompressed);
    return json == nullptr ? nullptr : env->NewStringUTF(json);
}

extern "C" JNIEXPORT jstring JNICALL Java_PressSdk_press_get_press_data_json(JNIEnv* env, jclass, jlong handle, jint isCompressed)
{
    const char* json = cpress_get_press_data_json(reinterpret_cast<CPressContext*>(handle), isCompressed);
    return json == nullptr ? nullptr : env->NewStringUTF(json);
}

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_set_press_data_json(JNIEnv* env, jclass, jlong handle, jstring json, jint isCompressed)
{
    if (json == nullptr)
    {
        return 0;
    }
    const char* utf8 = env->GetStringUTFChars(json, nullptr);
    if (utf8 == nullptr)
    {
        return 0;
    }
    int result = cpress_set_press_data_json(reinterpret_cast<CPressContext*>(handle), utf8, isCompressed);
    env->ReleaseStringUTFChars(json, utf8);
    return result;
}

extern "C" JNIEXPORT jstring JNICALL Java_PressSdk_press_get_real_time_data_json(JNIEnv* env, jclass, jlong handle, jint isCompressed)
{
    const char* json = cpress_get_real_time_data_json(reinterpret_cast<CPressContext*>(handle), isCompressed);
    return json == nullptr ? nullptr : env->NewStringUTF(json);
}

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_set_pressing(JNIEnv* env, jclass, jlong handle, jint isPressing)
{
    (void)env;
    return cpress_set_pressing(reinterpret_cast<CPressContext*>(handle), isPressing);
}

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_set_demolding(JNIEnv* env, jclass, jlong handle, jint isDemolding)
{
    (void)env;
    return cpress_set_demolding(reinterpret_cast<CPressContext*>(handle), isDemolding);
}

extern "C" JNIEXPORT jstring JNICALL Java_PressSdk_press_version(JNIEnv* env, jclass)
{
    return toJavaString(env, cpress_version());
}

extern "C" JNIEXPORT jstring JNICALL Java_PressSdk_press_get_serial_number(JNIEnv* env, jclass, jlong handle)
{
    char buffer[64] = {};
    int result = cpress_get_serial_number(reinterpret_cast<CPressContext*>(handle), buffer, sizeof(buffer));
    if (result == 0)
    {
        return toJavaString(env, "");
    }
    return toJavaString(env, buffer);
}

extern "C" JNIEXPORT void JNICALL Java_PressSdk_press_set_request_timeout(JNIEnv* env, jclass, jlong handle, jint ms)
{
    (void)env;
    cpress_set_request_timeout(reinterpret_cast<CPressContext*>(handle), static_cast<unsigned int>(ms));
}

extern "C" JNIEXPORT void JNICALL Java_PressSdk_press_register_data_interface(JNIEnv* env,
                                                                               jclass,
                                                                               jlong handle,
                                                                               jobject /*self*/,
                                                                               jobject callbacks,
                                                                               jint intervalSeconds)
{
    CPressContext* ctx = reinterpret_cast<CPressContext*>(handle);
    if (ctx == nullptr)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_callbackMapMutex);

        auto it = g_callbackMap.find(ctx);
        if (it != g_callbackMap.end())
        {
            // 先停 SDK 线程（join 后无在途回调），再清理 JNI 上下文，
            // 避免定时线程回调正在执行时 jniCtx 被释放造成 use-after-free
            cpress_unregister_data_interface(ctx);
            cleanupCallbackContext(env, it->second);
            g_callbackMap.erase(it);
        }
    }

    if (callbacks == nullptr)
    {
        return;
    }

    auto* jniCtx = new JniCallbackContext();
    env->GetJavaVM(&jniCtx->vm);

    jniCtx->callbacks = env->NewGlobalRef(callbacks);
    if (jniCtx->callbacks == nullptr)
    {
        delete jniCtx;
        return;
    }

    jclass callbackClass = env->GetObjectClass(callbacks);
    jniCtx->callbacksClass = static_cast<jclass>(env->NewGlobalRef(callbackClass));
    env->DeleteLocalRef(callbackClass);

    jniCtx->onReadOnlyDataMethod = env->GetMethodID(jniCtx->callbacksClass,
                                                     "onReadOnlyData",
                                                     "(IJLjava/lang/String;)V");
    jniCtx->onRealTimeDataMethod = env->GetMethodID(jniCtx->callbacksClass,
                                                     "onRealTimeData",
                                                     "(ILjava/lang/String;)V");
    jniCtx->onPressDataMethod = env->GetMethodID(jniCtx->callbacksClass,
                                                  "onPressData",
                                                  "(ILjava/lang/String;)V");
    jniCtx->onTempDataMethod = env->GetMethodID(jniCtx->callbacksClass,
                                                 "onTempData",
                                                 "(ILjava/lang/String;)V");
    jniCtx->onMixDataMethod = env->GetMethodID(jniCtx->callbacksClass,
                                                "onMixData",
                                                "(ILjava/lang/String;)V");
    jniCtx->onErrorMethod = env->GetMethodID(jniCtx->callbacksClass,
                                             "onError",
                                             "(I[B)V");
    clearPendingException(env);

    CPressDataCallbacks cCallbacks = {};
    cCallbacks.userData = jniCtx;
    cCallbacks.onReadOnlyData = jniOnReadOnlyData;
    cCallbacks.onRealTimeData = jniOnRealTimeData;
    cCallbacks.onPressData = jniOnPressData;
    cCallbacks.onTempData = jniOnTempData;
    cCallbacks.onMixData = jniOnMixData;
    cCallbacks.onError = jniOnError;

    cpress_register_data_interface(ctx, &cCallbacks, intervalSeconds);
    g_callbackMap[ctx] = jniCtx;
}

extern "C" JNIEXPORT void JNICALL Java_PressSdk_press_unregister_data_interface(JNIEnv* env,
                                                                                 jclass,
                                                                                 jlong handle)
{
    CPressContext* ctx = reinterpret_cast<CPressContext*>(handle);
    if (ctx == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(g_callbackMapMutex);

    auto it = g_callbackMap.find(ctx);
    if (it != g_callbackMap.end())
    {
        cleanupCallbackContext(env, it->second);
        g_callbackMap.erase(it);
    }

    cpress_unregister_data_interface(ctx);
}