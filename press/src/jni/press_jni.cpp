#include "jni.h"

#include "press/cpress.h"

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
constexpr const char* kPressSdkClassName = "PressSdk";
constexpr const char* kReadOnlyDataClassName = "PressSdk$ReadOnlyData";
constexpr const char* kPressDataClassName = "PressSdk$PressData";
constexpr const char* kRealTimeDataClassName = "PressSdk$RealTimeData";
constexpr const char* kDataCallbacksClassName = "PressSdk$DataCallbacks";

struct JniCallbackContext
{
    JavaVM* vm = nullptr;
    jobject callbacks = nullptr;
    jclass callbacksClass = nullptr;
    jmethodID onReadOnlyDataMethod = nullptr;
    jmethodID onRealTimeDataMethod = nullptr;
    jmethodID onPressDataMethod = nullptr;
    jmethodID onErrorMethod = nullptr;
};

static std::mutex g_callbackMapMutex;
static std::unordered_map<CPressContext*, JniCallbackContext*> g_callbackMap;

static void setStringField(JNIEnv* env, jobject object, const char* fieldName, const char* value)
{
    if (object == nullptr || value == nullptr)
    {
        return;
    }

    jclass clazz = env->GetObjectClass(object);
    if (clazz == nullptr)
    {
        return;
    }

    jfieldID fieldID = env->GetFieldID(clazz, fieldName, "Ljava/lang/String;");
    if (fieldID == nullptr)
    {
        env->DeleteLocalRef(clazz);
        return;
    }

    jstring jValue = env->NewStringUTF(value);
    env->SetObjectField(object, fieldID, jValue);
    env->DeleteLocalRef(jValue);
    env->DeleteLocalRef(clazz);
}

static void setByteField(JNIEnv* env, jobject object, const char* fieldName, uint8_t value)
{
    jclass clazz = env->GetObjectClass(object);
    if (clazz == nullptr)
    {
        return;
    }

    jfieldID fieldID = env->GetFieldID(clazz, fieldName, "B");
    if (fieldID != nullptr)
    {
        env->SetByteField(object, fieldID, static_cast<jbyte>(value));
    }
    env->DeleteLocalRef(clazz);
}

static void setLongField(JNIEnv* env, jobject object, const char* fieldName, int64_t value)
{
    jclass clazz = env->GetObjectClass(object);
    if (clazz == nullptr)
    {
        return;
    }

    jfieldID fieldID = env->GetFieldID(clazz, fieldName, "J");
    if (fieldID != nullptr)
    {
        env->SetLongField(object, fieldID, static_cast<jlong>(value));
    }
    env->DeleteLocalRef(clazz);
}

static void setFloatField(JNIEnv* env, jobject object, const char* fieldName, float value)
{
    jclass clazz = env->GetObjectClass(object);
    if (clazz == nullptr)
    {
        return;
    }

    jfieldID fieldID = env->GetFieldID(clazz, fieldName, "F");
    if (fieldID != nullptr)
    {
        env->SetFloatField(object, fieldID, value);
    }
    env->DeleteLocalRef(clazz);
}

static jstring toJavaString(JNIEnv* env, const char* text)
{
    if (text == nullptr)
    {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(text);
}

static void fillReadOnlyData(JNIEnv* env, jobject object, const ReadOnlyData& nativeData)
{
    setStringField(env, object, "m_NameZH", nativeData.m_NameZH);
    setStringField(env, object, "m_NameEN", nativeData.m_NameEN);
    setStringField(env, object, "m_Type", nativeData.m_Type);
    setStringField(env, object, "m_SerialNumber", nativeData.m_SerialNumber);
    setByteField(env, object, "m_Screenshot", nativeData.m_Screenshot);
    setByteField(env, object, "m_StartDelay", nativeData.m_StartDelay);
    setByteField(env, object, "m_VersionType", nativeData.m_VersionType);
    setByteField(env, object, "m_IsHideLang", nativeData.m_IsHideLang);
    setByteField(env, object, "m_Remote", nativeData.m_Remote);
    setByteField(env, object, "m_Network", nativeData.m_Network);
    setByteField(env, object, "m_FontZH", nativeData.m_FontZH);
    setByteField(env, object, "m_FontEN", nativeData.m_FontEN);
    setByteField(env, object, "m_MaxPStep", nativeData.m_MaxPStep);
    setFloatField(env, object, "m_MaxPLimit", nativeData.m_MaxPLimit);
    setFloatField(env, object, "m_MinPLimit", nativeData.m_MinPLimit);
    setFloatField(env, object, "m_Max_Min", nativeData.m_Max_Min);
    setFloatField(env, object, "m_Diameter", nativeData.m_Diameter);
    setByteField(env, object, "m_PDecimal", nativeData.m_PDecimal);
    setByteField(env, object, "m_PressDecimal", nativeData.m_PressDecimal);
    setByteField(env, object, "m_PModel", nativeData.m_PModel);
    setByteField(env, object, "m_OutType", nativeData.m_OutType);
    setByteField(env, object, "m_MaxTStep", nativeData.m_MaxTStep);
    setFloatField(env, object, "m_MaxTLimit", nativeData.m_MaxTLimit);
    setFloatField(env, object, "m_MinTLimit", nativeData.m_MinTLimit);
    setByteField(env, object, "m_TDecimal", nativeData.m_TDecimal);
    setByteField(env, object, "m_IsHasWater", nativeData.m_IsHasWater);
    setByteField(env, object, "m_IsHasSpeed", nativeData.m_IsHasSpeed);
}

static void fillPressData(JNIEnv* env, jobject object, const PressData& nativeData)
{
    setByteField(env, object, "m_PStep", nativeData.m_PStep);
    setByteField(env, object, "m_Type", nativeData.m_Type);
    setFloatField(env, object, "m_A", nativeData.m_A);
    setFloatField(env, object, "m_B", nativeData.m_B);
    setFloatField(env, object, "m_D", nativeData.m_D);
    setFloatField(env, object, "m_OuterD", nativeData.m_OuterD);
    setFloatField(env, object, "m_InnerD", nativeData.m_InnerD);
    setFloatField(env, object, "m_CheckValue", nativeData.m_CheckValue);
    setByteField(env, object, "m_Speed", nativeData.m_Speed);
    setFloatField(env, object, "m_DemoldValue", nativeData.m_DemoldValue);

    jclass clazz = env->GetObjectClass(object);
    if (clazz == nullptr)
    {
        return;
    }

    jfieldID setPField = env->GetFieldID(clazz, "m_SetPValue", "[F");
    jfieldID afterValueField = env->GetFieldID(clazz, "m_AfterValue", "[F");
    jfieldID kpTimeField = env->GetFieldID(clazz, "m_KPTime", "[I");
    if (setPField != nullptr)
    {
        jfloatArray setPArray = env->NewFloatArray(30);
        env->SetFloatArrayRegion(setPArray, 0, 30, nativeData.m_SetPValue);
        env->SetObjectField(object, setPField, setPArray);
        env->DeleteLocalRef(setPArray);
    }
    if (afterValueField != nullptr)
    {
        jfloatArray afterArray = env->NewFloatArray(30);
        env->SetFloatArrayRegion(afterArray, 0, 30, nativeData.m_AfterValue);
        env->SetObjectField(object, afterValueField, afterArray);
        env->DeleteLocalRef(afterArray);
    }
    if (kpTimeField != nullptr)
    {
        jintArray kpArray = env->NewIntArray(30);
        std::vector<jint> values(30);
        for (int i = 0; i < 30; ++i)
        {
            values[i] = static_cast<jint>(nativeData.m_KPTime[i]);
        }
        env->SetIntArrayRegion(kpArray, 0, 30, values.data());
        env->SetObjectField(object, kpTimeField, kpArray);
        env->DeleteLocalRef(kpArray);
    }
    env->DeleteLocalRef(clazz);
}

static void fillRealTimeData(JNIEnv* env, jobject object, const RealTimeData& nativeData)
{
    setByteField(env, object, "m_ModelState", nativeData.m_ModelState);
    setByteField(env, object, "m_PressState", nativeData.m_PressState);
    setByteField(env, object, "m_CPStep", nativeData.m_CPStep);
    setFloatField(env, object, "m_PressValue", nativeData.m_PressValue);
    setLongField(env, object, "m_PTime", nativeData.m_PTime);
    setByteField(env, object, "m_PdChanged", nativeData.m_PdChanged);
}

static void readPressData(JNIEnv* env, jobject object, PressData* nativeData)
{
    jclass clazz = env->GetObjectClass(object);
    if (clazz == nullptr)
    {
        return;
    }

    auto readByte = [&](const char* name, uint8_t* outValue) {
        jfieldID fieldID = env->GetFieldID(clazz, name, "B");
        if (fieldID != nullptr)
        {
            *outValue = static_cast<uint8_t>(env->GetByteField(object, fieldID));
        }
    };
    auto readFloat = [&](const char* name, float* outValue) {
        jfieldID fieldID = env->GetFieldID(clazz, name, "F");
        if (fieldID != nullptr)
        {
            *outValue = env->GetFloatField(object, fieldID);
        }
    };

    readByte("m_PStep", &nativeData->m_PStep);
    readByte("m_Type", &nativeData->m_Type);
    readFloat("m_A", &nativeData->m_A);
    readFloat("m_B", &nativeData->m_B);
    readFloat("m_D", &nativeData->m_D);
    readFloat("m_OuterD", &nativeData->m_OuterD);
    readFloat("m_InnerD", &nativeData->m_InnerD);
    readFloat("m_CheckValue", &nativeData->m_CheckValue);
    readByte("m_Speed", &nativeData->m_Speed);
    readFloat("m_DemoldValue", &nativeData->m_DemoldValue);

    jfieldID setPField = env->GetFieldID(clazz, "m_SetPValue", "[F");
    jfieldID afterField = env->GetFieldID(clazz, "m_AfterValue", "[F");
    jfieldID kpField = env->GetFieldID(clazz, "m_KPTime", "[I");

    if (setPField != nullptr)
    {
        jfloatArray array = static_cast<jfloatArray>(env->GetObjectField(object, setPField));
        if (array != nullptr)
        {
            env->GetFloatArrayRegion(array, 0, 30, nativeData->m_SetPValue);
            env->DeleteLocalRef(array);
        }
    }
    if (afterField != nullptr)
    {
        jfloatArray array = static_cast<jfloatArray>(env->GetObjectField(object, afterField));
        if (array != nullptr)
        {
            env->GetFloatArrayRegion(array, 0, 30, nativeData->m_AfterValue);
            env->DeleteLocalRef(array);
        }
    }
    if (kpField != nullptr)
    {
        jintArray array = static_cast<jintArray>(env->GetObjectField(object, kpField));
        if (array != nullptr)
        {
            std::vector<jint> values(30);
            env->GetIntArrayRegion(array, 0, 30, values.data());
            for (int i = 0; i < 30; ++i)
            {
                nativeData->m_KPTime[i] = static_cast<uint32_t>(values[i]);
            }
            env->DeleteLocalRef(array);
        }
    }
    env->DeleteLocalRef(clazz);
}

static jobject newJavaObject(JNIEnv* env, const char* className)
{
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr)
    {
        return nullptr;
    }

    jmethodID ctor = env->GetMethodID(clazz, "<init>", "()V");
    jobject instance = env->NewObject(clazz, ctor);
    env->DeleteLocalRef(clazz);
    return instance;
}

static JNIEnv* attachCurrentThread(JavaVM* vm)
{
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8) == JNI_EDETACHED)
    {
        if (vm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr) != JNI_OK)
        {
            return nullptr;
        }
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
                              const ReadOnlyData* readOnlyData)
{
    auto* ctx = static_cast<JniCallbackContext*>(userData);
    if (ctx == nullptr || ctx->vm == nullptr || ctx->callbacks == nullptr)
    {
        return;
    }

    JNIEnv* env = attachCurrentThread(ctx->vm);
    if (env == nullptr)
    {
        return;
    }

    jobject javaData = newJavaObject(env, kReadOnlyDataClassName);
    if (javaData != nullptr && readOnlyData != nullptr)
    {
        fillReadOnlyData(env, javaData, *readOnlyData);
    }

    if (ctx->onReadOnlyDataMethod != nullptr)
    {
        env->CallVoidMethod(ctx->callbacks,
                            ctx->onReadOnlyDataMethod,
                            errorCode,
                            static_cast<jlong>(registerNo),
                            javaData);
    }

    if (javaData != nullptr)
    {
        env->DeleteLocalRef(javaData);
    }
    detachCurrentThread(ctx->vm);
}

static void jniOnRealTimeData(void* userData,
                              int errorCode,
                              const RealTimeData* realTimeData)
{
    auto* ctx = static_cast<JniCallbackContext*>(userData);
    if (ctx == nullptr || ctx->vm == nullptr || ctx->callbacks == nullptr)
    {
        return;
    }

    JNIEnv* env = attachCurrentThread(ctx->vm);
    if (env == nullptr)
    {
        return;
    }

    jobject javaData = newJavaObject(env, kRealTimeDataClassName);
    if (javaData != nullptr && realTimeData != nullptr)
    {
        fillRealTimeData(env, javaData, *realTimeData);
    }

    if (ctx->onRealTimeDataMethod != nullptr)
    {
        env->CallVoidMethod(ctx->callbacks,
                            ctx->onRealTimeDataMethod,
                            errorCode,
                            javaData);
    }

    if (javaData != nullptr)
    {
        env->DeleteLocalRef(javaData);
    }
    detachCurrentThread(ctx->vm);
}

static void jniOnPressData(void* userData,
                           int errorCode,
                           const PressData* pressData)
{
    auto* ctx = static_cast<JniCallbackContext*>(userData);
    if (ctx == nullptr || ctx->vm == nullptr || ctx->callbacks == nullptr)
    {
        return;
    }

    JNIEnv* env = attachCurrentThread(ctx->vm);
    if (env == nullptr)
    {
        return;
    }

    jobject javaData = newJavaObject(env, kPressDataClassName);
    if (javaData != nullptr && pressData != nullptr)
    {
        fillPressData(env, javaData, *pressData);
    }

    if (ctx->onPressDataMethod != nullptr)
    {
        env->CallVoidMethod(ctx->callbacks,
                            ctx->onPressDataMethod,
                            errorCode,
                            javaData);
    }

    if (javaData != nullptr)
    {
        env->DeleteLocalRef(javaData);
    }
    detachCurrentThread(ctx->vm);
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

    JNIEnv* env = attachCurrentThread(ctx->vm);
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
    detachCurrentThread(ctx->vm);
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

    {
        std::lock_guard<std::mutex> lock(g_callbackMapMutex);
        auto it = g_callbackMap.find(ctx);
        if (it != g_callbackMap.end())
        {
            cleanupCallbackContext(env, it->second);
            g_callbackMap.erase(it);
        }
    }

    cpress_unregister_data_interface(ctx);
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

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_get_read_only_data(JNIEnv* env, jclass, jlong handle, jobject data, jint isCompressed)
{
    ReadOnlyData nativeData = {};
    int result = cpress_get_read_only_data(reinterpret_cast<CPressContext*>(handle), &nativeData, isCompressed);
    if (result != 0)
    {
        fillReadOnlyData(env, data, nativeData);
    }
    return result;
}

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_get_press_data(JNIEnv* env, jclass, jlong handle, jobject data, jint isCompressed)
{
    PressData nativeData = {};
    int result = cpress_get_press_data(reinterpret_cast<CPressContext*>(handle), &nativeData, isCompressed);
    if (result != 0)
    {
        fillPressData(env, data, nativeData);
    }
    return result;
}

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_set_press_data(JNIEnv* env, jclass, jlong handle, jobject data, jint isCompressed)
{
    PressData nativeData = {};
    readPressData(env, data, &nativeData);
    return cpress_set_press_data(reinterpret_cast<CPressContext*>(handle), &nativeData, isCompressed);
}

extern "C" JNIEXPORT jint JNICALL Java_PressSdk_press_get_real_time_data(JNIEnv* env, jclass, jlong handle, jobject data, jint isCompressed)
{
    RealTimeData nativeData = {};
    int result = cpress_get_real_time_data(reinterpret_cast<CPressContext*>(handle), &nativeData, isCompressed);
    if (result != 0)
    {
        fillRealTimeData(env, data, nativeData);
    }
    return result;
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

extern "C" JNIEXPORT void JNICALL Java_PressSdk_press_register_data_interface(JNIEnv* env,
                                                                               jclass,
                                                                               jlong handle,
                                                                               jobject /*self*/,
                                                                               jobject callbacks)
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
        cpress_unregister_data_interface(ctx);
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
                                                     "(IJLPressSdk$ReadOnlyData;)V");
    jniCtx->onRealTimeDataMethod = env->GetMethodID(jniCtx->callbacksClass,
                                                     "onRealTimeData",
                                                     "(ILPressSdk$RealTimeData;)V");
    jniCtx->onPressDataMethod = env->GetMethodID(jniCtx->callbacksClass,
                                                  "onPressData",
                                                  "(ILPressSdk$PressData;)V");
    jniCtx->onErrorMethod = env->GetMethodID(jniCtx->callbacksClass,
                                             "onError",
                                             "(I[B)V");

    CPressDataCallbacks cCallbacks = {};
    cCallbacks.userData = jniCtx;
    cCallbacks.onReadOnlyData = jniOnReadOnlyData;
    cCallbacks.onRealTimeData = jniOnRealTimeData;
    cCallbacks.onPressData = jniOnPressData;
    cCallbacks.onError = jniOnError;

    cpress_register_data_interface(ctx, &cCallbacks);
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