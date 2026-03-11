// core/jni/nexus_jni.cpp
// JNI Bridge for Android integration

#include "../platform/platform_abstraction.hpp"
#include "../engine/nexus_core.hpp"

#ifdef NEXUS_PLATFORM_ANDROID

#include <jni.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <string>
#include <vector>

#define LOG_TAG "NexusForge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global references
static JavaVM* g_javaVM = nullptr;
static jobject g_activity = nullptr;
static NexusForge::Core::NexusEngine* g_engine = nullptr;

// JNI Entry Points
extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_javaVM = vm;
    
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    
    LOGI("NexusForge JNI loaded");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("NexusForge JNI unloaded");
    
    if (g_engine) {
        g_engine->shutdown();
        delete g_engine;
        g_engine = nullptr;
    }
}

// NativeActivity callbacks
void onNativeActivityCreate(ANativeActivity* activity) {
    LOGI("NativeActivity created");
    
    // Store activity reference
    JNIEnv* env = activity->env;
    g_activity = env->NewGlobalRef(activity->clazz);
}

void onNativeActivityResume(ANativeActivity* activity) {
    LOGI("NativeActivity resumed");
    
    if (g_engine && !g_engine->isRunning()) {
        // Restart engine
    }
}

void onNativeActivityPause(ANativeActivity* activity) {
    LOGI("NativeActivity paused");
    
    if (g_engine) {
        // Pause engine
    }
}

void onNativeActivityDestroy(ANativeActivity* activity) {
    LOGI("NativeActivity destroyed");
    
    JNIEnv* env = activity->env;
    if (g_activity) {
        env->DeleteGlobalRef(g_activity);
        g_activity = nullptr;
    }
}

void onNativeActivityWindowFocusChanged(ANativeActivity* activity, int hasFocus) {
    LOGI("Window focus changed: %d", hasFocus);
}

void onNativeActivityConfigurationChanged(ANativeActivity* activity) {
    LOGI("Configuration changed");
}

void onNativeActivityLowMemory(ANativeActivity* activity) {
    LOGI("Low memory warning");
    
    if (g_engine) {
        // Trigger memory cleanup
    }
}

// JNI Methods called from Java
JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeInit(JNIEnv* env, jobject thiz) {
    LOGI("Native init called");
    
    // Store activity reference
    g_activity = env->NewGlobalRef(thiz);
    
    // Initialize engine
    g_engine = new NexusForge::Core::NexusEngine();
    
    NexusForge::Core::EngineConfig config;
    config.appName = "NexusForge IDE";
    config.appVersion = "1.0.0";
    config.dataPath = "/data/data/com.nexusforge.ide/files";
    config.cachePath = "/data/data/com.nexusforge.ide/cache";
    config.extensionsPath = "/data/data/com.nexusforge.ide/files/extensions";
    
    if (!g_engine->initialize(config)) {
        LOGE("Failed to initialize engine");
    }
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeRun(JNIEnv* env, jobject thiz) {
    LOGI("Native run called");
    
    if (g_engine) {
        g_engine->runMainLoop();
    }
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeShutdown(JNIEnv* env, jobject thiz) {
    LOGI("Native shutdown called");
    
    if (g_engine) {
        g_engine->shutdown();
    }
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativePause(JNIEnv* env, jobject thiz) {
    LOGI("Native pause called");
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeResume(JNIEnv* env, jobject thiz) {
    LOGI("Native resume called");
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeResize(JNIEnv* env, jobject thiz, 
                                                    jint width, jint height) {
    LOGI("Native resize: %dx%d", width, height);
    
    // Handle window resize
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeKeyEvent(JNIEnv* env, jobject thiz,
                                                      jint action, jint keyCode, jint metaState) {
    // Handle key events
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeTouchEvent(JNIEnv* env, jobject thiz,
                                                        jint action, jint pointerId,
                                                        jfloat x, jfloat y, jfloat pressure) {
    // Handle touch events
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeSetAssetManager(JNIEnv* env, jobject thiz,
                                                             jobject assetManager) {
    LOGI("Asset manager set");
    // Store asset manager for resource loading
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeSetDataPath(JNIEnv* env, jobject thiz,
                                                         jstring internalPath, 
                                                         jstring externalPath) {
    const char* internal = env->GetStringUTFChars(internalPath, nullptr);
    const char* external = env->GetStringUTFChars(externalPath, nullptr);
    
    LOGI("Data paths: internal=%s, external=%s", internal, external);
    
    env->ReleaseStringUTFChars(internalPath, internal);
    env->ReleaseStringUTFChars(externalPath, external);
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeOpenFile(JNIEnv* env, jobject thiz,
                                                      jstring path) {
    const char* filePath = env->GetStringUTFChars(path, nullptr);
    LOGI("Opening file: %s", filePath);
    
    // Open file in editor
    
    env->ReleaseStringUTFChars(path, filePath);
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeKeyboardVisibilityChanged(JNIEnv* env, 
                                                                       jobject thiz,
                                                                       jboolean visible,
                                                                       jint height) {
    LOGI("Keyboard %s, height=%d", visible ? "shown" : "hidden", height);
    
    // Adjust UI for keyboard
}

JNIEXPORT jstring JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeGetClipboardText(JNIEnv* env, jobject thiz) {
    // Get clipboard text
    return env->NewStringUTF("");
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeSetClipboardText(JNIEnv* env, jobject thiz,
                                                              jstring text) {
    const char* textStr = env->GetStringUTFChars(text, nullptr);
    // Set clipboard text
    env->ReleaseStringUTFChars(text, textStr);
}

} // extern "C"

#endif // NEXUS_PLATFORM_ANDROID
