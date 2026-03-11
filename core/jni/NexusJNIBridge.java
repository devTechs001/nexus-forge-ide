// core/jni/NexusJNIBridge.java
// Java JNI Bridge for Android

package com.nexusforge.ide;

import android.app.NativeActivity;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Build;
import android.os.Bundle;
import android.view.View;

public class NexusJNIBridge extends NativeActivity {

    // Native methods
    private native void nativeInit();
    private native void nativeRun();
    private native void nativeShutdown();
    private native void nativePause();
    private native void nativeResume();
    private native void nativeResize(int width, int height);
    private native void nativeKeyEvent(int action, int keyCode, int metaState);
    private native void nativeTouchEvent(int action, int pointerId, float x, float y, float pressure);
    private native void nativeSetAssetManager(AssetManager assetManager);
    private native void nativeSetDataPath(String internalPath, String externalPath);
    private native void nativeOpenFile(String path);
    private native void nativeKeyboardVisibilityChanged(boolean visible, int height);
    private native String nativeGetClipboardText();
    private native void nativeSetClipboardText(String text);

    // Load native library
    static {
        System.loadLibrary("nexusforge");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Pass asset manager to native code
        nativeSetAssetManager(getAssets());

        // Pass data paths
        String internalPath = getFilesDir().getAbsolutePath();
        String externalPath = getExternalFilesDir(null).getAbsolutePath();
        nativeSetDataPath(internalPath, externalPath);

        // Initialize native code
        nativeInit();
    }

    @Override
    protected void onResume() {
        super.onResume();
        nativeResume();
    }

    @Override
    protected void onPause() {
        nativePause();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        nativeShutdown();
        super.onDestroy();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        
        if (hasFocus) {
            View decorView = getWindow().getDecorView();
            decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            );
        }
    }

    // Called from native code to show toast
    public void showToast(final String message) {
        runOnUiThread(() -> {
            android.widget.Toast.makeText(this, message, android.widget.Toast.LENGTH_SHORT).show();
        });
    }

    // Called from native code to open URL
    public void openUrl(final String url) {
        runOnUiThread(() -> {
            android.content.Intent intent = new android.content.Intent(android.content.Intent.ACTION_VIEW);
            intent.setData(android.net.Uri.parse(url));
            startActivity(intent);
        });
    }

    // Called from native code to get device info
    public DeviceInfo getDeviceInfo() {
        DeviceInfo info = new DeviceInfo();
        
        View decorView = getWindow().getDecorView();
        info.screenWidth = decorView.getWidth();
        info.screenHeight = decorView.getHeight();
        info.density = getResources().getDisplayMetrics().density;
        
        // Safe area insets (API 29+)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            android.graphics.Insets insets = decorView.getRootWindowInsets()
                .getInsets(android.view.WindowInsets.Type.systemBars());
            info.safeAreaTop = insets.top;
            info.safeAreaBottom = insets.bottom;
            info.safeAreaLeft = insets.left;
            info.safeAreaRight = insets.right;
        }
        
        return info;
    }

    public static class DeviceInfo {
        public int screenWidth;
        public int screenHeight;
        public float density;
        public int safeAreaTop;
        public int safeAreaBottom;
        public int safeAreaLeft;
        public int safeAreaRight;
    }
}
