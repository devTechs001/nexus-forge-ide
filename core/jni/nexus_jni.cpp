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

    // Replace any previous activity reference safely
    if (g_activity) {
        env->DeleteGlobalRef(g_activity);
        g_activity = nullptr;
    }
    if (thiz) {
        g_activity = env->NewGlobalRef(thiz);
    }

    // Replace any previous engine instance cleanly
    if (g_engine) {
        g_engine->shutdown();
        delete g_engine;
        g_engine = nullptr;
    }

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
        delete g_engine;
        g_engine = nullptr;
    }
    if (g_activity) {
        env->DeleteGlobalRef(g_activity);
        g_activity = nullptr;
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
    const char* internal = internalPath ? env->GetStringUTFChars(internalPath, nullptr) : nullptr;
    const char* external = externalPath ? env->GetStringUTFChars(externalPath, nullptr) : nullptr;

    LOGI("Data paths: internal=%s, external=%s",
         internal ? internal : "",
         external ? external : "");

    if (internalPath && internal) env->ReleaseStringUTFChars(internalPath, internal);
    if (externalPath && external) env->ReleaseStringUTFChars(externalPath, external);
}

JNIEXPORT void JNICALL
Java_com_nexusforge_ide_NexusActivity_nativeOpenFile(JNIEnv* env, jobject thiz,
                                                      jstring path) {
    if (!path) return;
    const char* filePath = env->GetStringUTFChars(path, nullptr);
    if (!filePath) return;
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
    if (!text) return;
    const char* textStr = env->GetStringUTFChars(text, nullptr);
    if (!textStr) return;
    // Set clipboard text
    env->ReleaseStringUTFChars(text, textStr);
}

// Matching JNI exports for NexusJNIBridge class (delegating to NexusActivity handlers)
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeInit(JNIEnv* env, jobject thiz) {
    Java_com_nexusforge_ide_NexusActivity_nativeInit(env, thiz);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeRun(JNIEnv* env, jobject thiz) {
    Java_com_nexusforge_ide_NexusActivity_nativeRun(env, thiz);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeShutdown(JNIEnv* env, jobject thiz) {
    Java_com_nexusforge_ide_NexusActivity_nativeShutdown(env, thiz);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativePause(JNIEnv* env, jobject thiz) {
    Java_com_nexusforge_ide_NexusActivity_nativePause(env, thiz);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeResume(JNIEnv* env, jobject thiz) {
    Java_com_nexusforge_ide_NexusActivity_nativeResume(env, thiz);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeResize(JNIEnv* env, jobject thiz, jint width, jint height) {
    Java_com_nexusforge_ide_NexusActivity_nativeResize(env, thiz, width, height);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeKeyEvent(JNIEnv* env, jobject thiz, jint action, jint keyCode, jint metaState) {
    Java_com_nexusforge_ide_NexusActivity_nativeKeyEvent(env, thiz, action, keyCode, metaState);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeTouchEvent(JNIEnv* env, jobject thiz, jint action, jint pointerId, jfloat x, jfloat y, jfloat pressure) {
    Java_com_nexusforge_ide_NexusActivity_nativeTouchEvent(env, thiz, action, pointerId, x, y, pressure);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeSetAssetManager(JNIEnv* env, jobject thiz, jobject assetManager) {
    Java_com_nexusforge_ide_NexusActivity_nativeSetAssetManager(env, thiz, assetManager);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeSetDataPath(JNIEnv* env, jobject thiz, jstring internalPath, jstring externalPath) {
    Java_com_nexusforge_ide_NexusActivity_nativeSetDataPath(env, thiz, internalPath, externalPath);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeOpenFile(JNIEnv* env, jobject thiz, jstring path) {
    Java_com_nexusforge_ide_NexusActivity_nativeOpenFile(env, thiz, path);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeKeyboardVisibilityChanged(JNIEnv* env, jobject thiz, jboolean visible, jint height) {
    Java_com_nexusforge_ide_NexusActivity_nativeKeyboardVisibilityChanged(env, thiz, visible, height);
}
JNIEXPORT jstring JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeGetClipboardText(JNIEnv* env, jobject thiz) {
    return Java_com_nexusforge_ide_NexusActivity_nativeGetClipboardText(env, thiz);
}
JNIEXPORT void JNICALL Java_com_nexusforge_ide_NexusJNIBridge_nativeSetClipboardText(JNIEnv* env, jobject thiz, jstring text) {
    Java_com_nexusforge_ide_NexusActivity_nativeSetClipboardText(env, thiz, text);
}

} // extern "C"

#endif // NEXUS_PLATFORM_ANDROID
