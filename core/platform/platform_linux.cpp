// core/platform/platform_linux.cpp
#include "platform_abstraction.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>

#ifdef NEXUS_PLATFORM_LINUX

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <array>
#include <algorithm>
#include <mutex>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <climits>

// Optional system X11 headers (we still dlopen libX11.so.6 so there is no hard link requirement)
#if defined(__has_include)
    #if __has_include(<X11/Xlib.h>) && __has_include(<X11/Xutil.h>) && __has_include(<X11/Xatom.h>) && __has_include(<X11/keysym.h>)
        #define NEXUS_HAS_X11_HEADERS 1
        #include <X11/Xlib.h>
        #include <X11/Xutil.h>
        #include <X11/Xatom.h>
        #include <X11/keysym.h>
    #endif
#endif

namespace {

#ifdef NEXUS_HAS_X11_HEADERS
    using X11Display = ::Display;
    using X11Window = ::Window;
    using X11Visual = ::Visual;
    using X11GC = ::GC;
    using X11Atom = ::Atom;
    using X11Image = ::XImage;
    using X11Event = ::XEvent;
    using X11KeyEvent = ::XKeyEvent;
    using X11KeySym = ::KeySym;
    using X11SizeHints = ::XSizeHints;
    using X11ComposeStatus = ::XComposeStatus;

    constexpr int X11_KeyPress        = KeyPress;
    constexpr int X11_KeyRelease      = KeyRelease;
    constexpr int X11_ButtonPress     = ButtonPress;
    constexpr int X11_ButtonRelease   = ButtonRelease;
    constexpr int X11_MotionNotify    = MotionNotify;
    constexpr int X11_Expose          = Expose;
    constexpr int X11_ConfigureNotify = ConfigureNotify;
    constexpr int X11_ClientMessage   = ClientMessage;
    constexpr int X11_ZPixmap         = ZPixmap;
    constexpr int X11_PropModeReplace = PropModeReplace;
    constexpr unsigned long X11_XA_ATOM = XA_ATOM;
    constexpr long X11_EventMask = ExposureMask | StructureNotifyMask |
                                   KeyPressMask | KeyReleaseMask |
                                   ButtonPressMask | ButtonReleaseMask |
                                   PointerMotionMask;
    constexpr long X11_SubstructureRedirectMask = SubstructureRedirectMask;
    constexpr long X11_SubstructureNotifyMask   = SubstructureNotifyMask;
    constexpr unsigned int X11_ShiftMask   = ShiftMask;
    constexpr unsigned int X11_ControlMask = ControlMask;
    constexpr unsigned int X11_Mod1Mask    = Mod1Mask;
    constexpr long X11_USPosition = USPosition;
    constexpr long X11_PPosition  = PPosition;
    constexpr long X11_PSize      = PSize;
    constexpr long X11_PMinSize   = PMinSize;
    constexpr long X11_PMaxSize   = PMaxSize;

    // Undefine X11 macros that collide with C++ enum identifiers
    #ifdef None
        #undef None
    #endif
    #ifdef KeyPress
        #undef KeyPress
    #endif
    #ifdef KeyRelease
        #undef KeyRelease
    #endif
    #ifdef Bool
        #undef Bool
    #endif
    #ifdef Status
        #undef Status
    #endif
    #ifdef Success
        #undef Success
    #endif
    #ifdef Always
        #undef Always
    #endif
    #ifdef True
        #undef True
    #endif
    #ifdef False
        #undef False
    #endif
    #ifdef CurrentTime
        #undef CurrentTime
    #endif
    #ifdef XDestroyImage
        #undef XDestroyImage
    #endif
#else
    // Stable X11R6 C ABI definitions for environments without libx11-dev headers
    struct X11Display;
    struct X11Visual;
    struct X11GCOpaque;
    struct X11Image;
    struct X11ComposeStatus;
    using X11Window = unsigned long;
    using X11Drawable = unsigned long;
    using X11Atom = unsigned long;
    using X11KeySym = unsigned long;
    using X11GC = X11GCOpaque*;
    using X11Time = unsigned long;

    struct X11AnyEvent {
        int type;
        unsigned long serial;
        int send_event;
        X11Display* display;
        X11Window window;
    };

    struct X11KeyEvent {
        int type;
        unsigned long serial;
        int send_event;
        X11Display* display;
        X11Window window;
        X11Window root;
        X11Window subwindow;
        X11Time time;
        int x, y;
        int x_root, y_root;
        unsigned int state;
        unsigned int keycode;
        int same_screen;
    };

    struct X11ButtonEvent {
        int type;
        unsigned long serial;
        int send_event;
        X11Display* display;
        X11Window window;
        X11Window root;
        X11Window subwindow;
        X11Time time;
        int x, y;
        int x_root, y_root;
        unsigned int state;
        unsigned int button;
        int same_screen;
    };

    struct X11MotionEvent {
        int type;
        unsigned long serial;
        int send_event;
        X11Display* display;
        X11Window window;
        X11Window root;
        X11Window subwindow;
        X11Time time;
        int x, y;
        int x_root, y_root;
        unsigned int state;
        char is_hint;
        int same_screen;
    };

    struct X11ExposeEvent {
        int type;
        unsigned long serial;
        int send_event;
        X11Display* display;
        X11Window window;
        int x, y;
        int width, height;
        int count;
    };

    struct X11ConfigureEvent {
        int type;
        unsigned long serial;
        int send_event;
        X11Display* display;
        X11Window event;
        X11Window window;
        int x, y;
        int width, height;
        int border_width;
        X11Window above;
        int override_redirect;
    };

    struct X11ClientMessageEvent {
        int type;
        unsigned long serial;
        int send_event;
        X11Display* display;
        X11Window window;
        X11Atom message_type;
        int format;
        union {
            char b[20];
            short s[10];
            long l[5];
        } data;
    };

    union X11Event {
        int type;
        X11AnyEvent xany;
        X11KeyEvent xkey;
        X11ButtonEvent xbutton;
        X11MotionEvent xmotion;
        X11ExposeEvent xexpose;
        X11ConfigureEvent xconfigure;
        X11ClientMessageEvent xclient;
        long pad[24];
    };

    struct X11SizeHints {
        long flags;
        int x, y;
        int width, height;
        int min_width, min_height;
        int max_width, max_height;
        int width_inc, height_inc;
        struct { int x; int y; } min_aspect, max_aspect;
        int base_width, base_height;
        int win_gravity;
    };

    constexpr int X11_KeyPress        = 2;
    constexpr int X11_KeyRelease      = 3;
    constexpr int X11_ButtonPress     = 4;
    constexpr int X11_ButtonRelease   = 5;
    constexpr int X11_MotionNotify    = 6;
    constexpr int X11_Expose          = 12;
    constexpr int X11_ConfigureNotify = 22;
    constexpr int X11_ClientMessage   = 33;
    constexpr int X11_ZPixmap         = 2;
    constexpr int X11_PropModeReplace = 0;
    constexpr unsigned long X11_XA_ATOM = 4;
    constexpr long X11_EventMask = (1L << 0) | (1L << 1) | (1L << 2) | (1L << 3) |
                                   (1L << 6) | (1L << 15) | (1L << 17);
    constexpr long X11_SubstructureRedirectMask = (1L << 20);
    constexpr long X11_SubstructureNotifyMask   = (1L << 19);
    constexpr unsigned int X11_ShiftMask   = (1U << 0);
    constexpr unsigned int X11_ControlMask = (1U << 2);
    constexpr unsigned int X11_Mod1Mask    = (1U << 3);
    constexpr long X11_USPosition = (1L << 0);
    constexpr long X11_PPosition  = (1L << 2);
    constexpr long X11_PSize      = (1L << 3);
    constexpr long X11_PMinSize   = (1L << 4);
    constexpr long X11_PMaxSize   = (1L << 5);
#endif

// Standard X11 keysyms (constant across all X11 versions)
constexpr X11KeySym X11_XK_BackSpace = 0xff08;
constexpr X11KeySym X11_XK_Tab       = 0xff09;
constexpr X11KeySym X11_XK_Return    = 0xff0d;
constexpr X11KeySym X11_XK_Escape    = 0xff1b;
constexpr X11KeySym X11_XK_Home      = 0xff50;
constexpr X11KeySym X11_XK_Left      = 0xff51;
constexpr X11KeySym X11_XK_Up        = 0xff52;
constexpr X11KeySym X11_XK_Right     = 0xff53;
constexpr X11KeySym X11_XK_Down      = 0xff54;
constexpr X11KeySym X11_XK_Page_Up   = 0xff55;
constexpr X11KeySym X11_XK_Page_Down = 0xff56;
constexpr X11KeySym X11_XK_End       = 0xff57;
constexpr X11KeySym X11_XK_Delete    = 0xffff;
constexpr X11KeySym X11_XK_KP_Enter  = 0xff8d;

template<typename Fn>
bool loadX11Sym(void* lib, const char* name, Fn& out) {
    void* sym = dlsym(lib, name);
    if (!sym) return false;
    std::memcpy(&out, &sym, sizeof(out));
    return true;
}

struct X11Api {
    void* lib = nullptr;
    bool loaded = false;

    X11Display* (*fnXOpenDisplay)(const char*) = nullptr;
    int (*fnXCloseDisplay)(X11Display*) = nullptr;
    int (*fnXDefaultScreen)(X11Display*) = nullptr;
    X11Window (*fnXRootWindow)(X11Display*, int) = nullptr;
    X11Visual* (*fnXDefaultVisual)(X11Display*, int) = nullptr;
    int (*fnXDefaultDepth)(X11Display*, int) = nullptr;
    int (*fnXDisplayWidth)(X11Display*, int) = nullptr;
    int (*fnXDisplayHeight)(X11Display*, int) = nullptr;
    unsigned long (*fnXBlackPixel)(X11Display*, int) = nullptr;
    X11Window (*fnXCreateSimpleWindow)(X11Display*, X11Window, int, int, unsigned int, unsigned int,
                                       unsigned int, unsigned long, unsigned long) = nullptr;
    int (*fnXDestroyWindow)(X11Display*, X11Window) = nullptr;
    int (*fnXSelectInput)(X11Display*, X11Window, long) = nullptr;
    int (*fnXMapRaised)(X11Display*, X11Window) = nullptr;
    int (*fnXUnmapWindow)(X11Display*, X11Window) = nullptr;
    int (*fnXStoreName)(X11Display*, X11Window, const char*) = nullptr;
    int (*fnXMoveWindow)(X11Display*, X11Window, int, int) = nullptr;
    int (*fnXResizeWindow)(X11Display*, X11Window, unsigned int, unsigned int) = nullptr;
    int (*fnXIconifyWindow)(X11Display*, X11Window, int) = nullptr;
    int (*fnXFlush)(X11Display*) = nullptr;
    int (*fnXSync)(X11Display*, int) = nullptr;
    int (*fnXPending)(X11Display*) = nullptr;
    int (*fnXNextEvent)(X11Display*, X11Event*) = nullptr;
    X11Atom (*fnXInternAtom)(X11Display*, const char*, int) = nullptr;
    int (*fnXSetWMProtocols)(X11Display*, X11Window, X11Atom*, int) = nullptr;
    int (*fnXChangeProperty)(X11Display*, X11Window, X11Atom, X11Atom, int, int,
                             const unsigned char*, int) = nullptr;
    int (*fnXSendEvent)(X11Display*, X11Window, int, long, X11Event*) = nullptr;
    X11GC (*fnXCreateGC)(X11Display*, X11Window, unsigned long, void*) = nullptr;
    int (*fnXFreeGC)(X11Display*, X11GC) = nullptr;
    X11Image* (*fnXCreateImage)(X11Display*, X11Visual*, unsigned int, int, int, char*,
                                unsigned int, unsigned int, int, int) = nullptr;
    int (*fnXDestroyImage)(X11Image*) = nullptr;
    int (*fnXPutImage)(X11Display*, X11Window, X11GC, X11Image*, int, int, int, int,
                       unsigned int, unsigned int) = nullptr;
    int (*fnXLookupString)(X11KeyEvent*, char*, int, X11KeySym*, X11ComposeStatus*) = nullptr;
    X11KeySym (*fnXLookupKeysym)(X11KeyEvent*, int) = nullptr;
    X11SizeHints* (*fnXAllocSizeHints)() = nullptr;
    void (*fnXSetWMNormalHints)(X11Display*, X11Window, X11SizeHints*) = nullptr;
    int (*fnXFree)(void*) = nullptr;

    bool init() {
        if (loaded) return true;
        lib = dlopen("libX11.so.6", RTLD_LAZY);
        if (!lib) lib = dlopen("libX11.so", RTLD_LAZY);
        if (!lib) return false;

        bool ok = true;
        ok &= loadX11Sym(lib, "XOpenDisplay", fnXOpenDisplay);
        ok &= loadX11Sym(lib, "XCloseDisplay", fnXCloseDisplay);
        ok &= loadX11Sym(lib, "XDefaultScreen", fnXDefaultScreen);
        ok &= loadX11Sym(lib, "XRootWindow", fnXRootWindow);
        ok &= loadX11Sym(lib, "XDefaultVisual", fnXDefaultVisual);
        ok &= loadX11Sym(lib, "XDefaultDepth", fnXDefaultDepth);
        ok &= loadX11Sym(lib, "XDisplayWidth", fnXDisplayWidth);
        ok &= loadX11Sym(lib, "XDisplayHeight", fnXDisplayHeight);
        ok &= loadX11Sym(lib, "XBlackPixel", fnXBlackPixel);
        ok &= loadX11Sym(lib, "XCreateSimpleWindow", fnXCreateSimpleWindow);
        ok &= loadX11Sym(lib, "XDestroyWindow", fnXDestroyWindow);
        ok &= loadX11Sym(lib, "XSelectInput", fnXSelectInput);
        ok &= loadX11Sym(lib, "XMapRaised", fnXMapRaised);
        ok &= loadX11Sym(lib, "XUnmapWindow", fnXUnmapWindow);
        ok &= loadX11Sym(lib, "XStoreName", fnXStoreName);
        ok &= loadX11Sym(lib, "XMoveWindow", fnXMoveWindow);
        ok &= loadX11Sym(lib, "XResizeWindow", fnXResizeWindow);
        ok &= loadX11Sym(lib, "XIconifyWindow", fnXIconifyWindow);
        ok &= loadX11Sym(lib, "XFlush", fnXFlush);
        ok &= loadX11Sym(lib, "XSync", fnXSync);
        ok &= loadX11Sym(lib, "XPending", fnXPending);
        ok &= loadX11Sym(lib, "XNextEvent", fnXNextEvent);
        ok &= loadX11Sym(lib, "XInternAtom", fnXInternAtom);
        ok &= loadX11Sym(lib, "XSetWMProtocols", fnXSetWMProtocols);
        ok &= loadX11Sym(lib, "XChangeProperty", fnXChangeProperty);
        ok &= loadX11Sym(lib, "XSendEvent", fnXSendEvent);
        ok &= loadX11Sym(lib, "XCreateGC", fnXCreateGC);
        ok &= loadX11Sym(lib, "XFreeGC", fnXFreeGC);
        ok &= loadX11Sym(lib, "XCreateImage", fnXCreateImage);
        ok &= loadX11Sym(lib, "XDestroyImage", fnXDestroyImage);
        ok &= loadX11Sym(lib, "XPutImage", fnXPutImage);
        ok &= loadX11Sym(lib, "XLookupString", fnXLookupString);
        ok &= loadX11Sym(lib, "XLookupKeysym", fnXLookupKeysym);
        // Optional helpers
        loadX11Sym(lib, "XAllocSizeHints", fnXAllocSizeHints);
        loadX11Sym(lib, "XSetWMNormalHints", fnXSetWMNormalHints);
        loadX11Sym(lib, "XFree", fnXFree);

        if (!ok) {
            dlclose(lib);
            lib = nullptr;
            return false;
        }
        loaded = true;
        return true;
    }

    void shutdown() {
        if (lib) {
            dlclose(lib);
            lib = nullptr;
        }
        loaded = false;
    }
};

struct LinuxWindow {
    X11Window xwin = 0;
    X11GC gc = nullptr;
    X11Image* ximage = nullptr;
    uint32_t* ximagePixels = nullptr;
    int imageWidth = 0;
    int imageHeight = 0;
    int width = 800;
    int height = 600;
    bool isSplash = false;
    bool visible = false;
    bool fullscreen = false;
    bool closeRequested = false;
    bool hasNativeX11 = false;
};

struct LinuxDisplayState {
    X11Api x11;
    X11Display* display = nullptr;
    int screen = 0;
    int depth = 24;
    X11Visual* visual = nullptr;
    X11Atom wmDeleteWindow = 0;
    X11Atom netWmState = 0;
    X11Atom netWmStateFullscreen = 0;
    X11Atom netWmStateMaxHorz = 0;
    X11Atom netWmStateMaxVert = 0;
    X11Atom netWmWindowType = 0;
    X11Atom netWmWindowTypeSplash = 0;
    X11Atom motifWmHints = 0;
    X11Atom netWmName = 0;
    X11Atom utf8String = 0;
    std::vector<LinuxWindow*> windows;
    NexusForge::Platform::PlatformAbstraction::WindowEventCallback eventCallback;
    std::mutex mutex;

    bool initDisplay() {
        if (display) return true;
        if (!x11.init()) return false;
        display = x11.fnXOpenDisplay(nullptr);
        if (!display) return false;

        screen = x11.fnXDefaultScreen(display);
        depth = x11.fnXDefaultDepth(display, screen);
        visual = x11.fnXDefaultVisual(display, screen);

        wmDeleteWindow = x11.fnXInternAtom(display, "WM_DELETE_WINDOW", 0);
        netWmState = x11.fnXInternAtom(display, "_NET_WM_STATE", 0);
        netWmStateFullscreen = x11.fnXInternAtom(display, "_NET_WM_STATE_FULLSCREEN", 0);
        netWmStateMaxHorz = x11.fnXInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
        netWmStateMaxVert = x11.fnXInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
        netWmWindowType = x11.fnXInternAtom(display, "_NET_WM_WINDOW_TYPE", 0);
        netWmWindowTypeSplash = x11.fnXInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
        motifWmHints = x11.fnXInternAtom(display, "_MOTIF_WM_HINTS", 0);
        netWmName = x11.fnXInternAtom(display, "_NET_WM_NAME", 0);
        utf8String = x11.fnXInternAtom(display, "UTF8_STRING", 0);
        return true;
    }

    LinuxWindow* findByXWindow(X11Window xwin) {
        for (auto* w : windows) {
            if (w && w->hasNativeX11 && w->xwin == xwin) return w;
        }
        return nullptr;
    }

    void destroyImage(LinuxWindow* win) {
        if (win && win->ximage) {
            x11.fnXDestroyImage(win->ximage); // frees both XImage and ximagePixels
            win->ximage = nullptr;
            win->ximagePixels = nullptr;
            win->imageWidth = 0;
            win->imageHeight = 0;
        }
    }

    void sendNetWmState(LinuxWindow* win, long action, X11Atom atom1, X11Atom atom2 = 0) {
        if (!display || !win || !win->hasNativeX11 || !netWmState) return;
        X11Event ev{};
        ev.xclient.type = X11_ClientMessage;
        ev.xclient.window = win->xwin;
        ev.xclient.message_type = netWmState;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = action; // 0=remove, 1=add, 2=toggle
        ev.xclient.data.l[1] = static_cast<long>(atom1);
        ev.xclient.data.l[2] = static_cast<long>(atom2);
        ev.xclient.data.l[3] = 1; // normal application source
        x11.fnXSendEvent(display, x11.fnXRootWindow(display, screen), 0,
                         X11_SubstructureRedirectMask | X11_SubstructureNotifyMask, &ev);
        x11.fnXFlush(display);
    }

    void shutdownDisplay() {
        for (auto* win : windows) {
            if (!win) continue;
            destroyImage(win);
            if (display && win->hasNativeX11) {
                if (win->gc) x11.fnXFreeGC(display, win->gc);
                if (win->xwin) x11.fnXDestroyWindow(display, win->xwin);
            }
            delete win;
        }
        windows.clear();
        if (display) {
            x11.fnXCloseDisplay(display);
            display = nullptr;
        }
        x11.shutdown();
    }
};

LinuxDisplayState& getDisplayState() {
    static LinuxDisplayState state;
    return state;
}

int translateModifiers(unsigned int xstate) {
    int mods = 0;
    if (xstate & X11_ShiftMask)   mods |= 0x0001;
    if (xstate & X11_ControlMask) mods |= 0x0002;
    if (xstate & X11_Mod1Mask)    mods |= 0x0004;
    return mods;
}

int translateKeySym(X11KeySym ks) {
    switch (ks) {
        case X11_XK_BackSpace: return NexusForge::Platform::KeyCode::Backspace;
        case X11_XK_Tab:       return NexusForge::Platform::KeyCode::Tab;
        case X11_XK_Return:
        case X11_XK_KP_Enter:  return NexusForge::Platform::KeyCode::Enter;
        case X11_XK_Escape:    return NexusForge::Platform::KeyCode::Escape;
        case X11_XK_Delete:    return NexusForge::Platform::KeyCode::Delete;
        case X11_XK_Left:      return NexusForge::Platform::KeyCode::Left;
        case X11_XK_Right:     return NexusForge::Platform::KeyCode::Right;
        case X11_XK_Up:        return NexusForge::Platform::KeyCode::Up;
        case X11_XK_Down:      return NexusForge::Platform::KeyCode::Down;
        case X11_XK_Home:      return NexusForge::Platform::KeyCode::Home;
        case X11_XK_End:       return NexusForge::Platform::KeyCode::End;
        case X11_XK_Page_Up:   return NexusForge::Platform::KeyCode::PageUp;
        case X11_XK_Page_Down: return NexusForge::Platform::KeyCode::PageDown;
        default: break;
    }
    if (ks >= 'a' && ks <= 'z') {
        return static_cast<int>(ks - 'a' + 'A'); // Uppercase canonical keyCode for shortcuts
    }
    if (ks >= 32 && ks < 127) {
        return static_cast<int>(ks);
    }
    return 0;
}

} // anonymous namespace

namespace NexusForge::Platform {

PlatformType PlatformAbstraction::currentPlatform_ = PlatformType::Linux;
bool PlatformAbstraction::initialized_ = false;

bool PlatformAbstraction::initialize() {
    initialized_ = true;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.initDisplay(); // Gracefully optional: returns false in headless environments
    return true;
}

void PlatformAbstraction::shutdown() {
    {
        auto& state = getDisplayState();
        std::lock_guard<std::mutex> lock(state.mutex);
        state.eventCallback = nullptr;
        state.shutdownDisplay();
    }
    initialized_ = false;
}

PlatformType PlatformAbstraction::getCurrentPlatform() {
    return currentPlatform_;
}

SystemInfo PlatformAbstraction::getSystemInfo() {
    SystemInfo info;
    info.platform = PlatformType::Linux;
    
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        info.osName = unameData.sysname;
        info.osVersion = unameData.release;
        info.architecture = unameData.machine;
    }
    
    info.cpuCount = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
    
    struct sysinfo sysInfo;
    if (sysinfo(&sysInfo) == 0) {
        info.totalMemory = sysInfo.totalram * sysInfo.mem_unit;
        info.availableMemory = sysInfo.freeram * sysInfo.mem_unit;
    }
    
    return info;
}

DirectoryPaths PlatformAbstraction::getDirectoryPaths() {
    DirectoryPaths paths;
    
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_dir) {
            home = pw->pw_dir;
        }
    }
    paths.homeDir = home ? home : "/tmp";
    
    paths.dataDir = paths.homeDir + "/.local/share";
    paths.configDir = paths.homeDir + "/.config";
    paths.cacheDir = paths.homeDir + "/.cache";
    paths.tempDir = "/tmp";
    paths.documentsDir = paths.homeDir + "/Documents";
    paths.downloadsDir = paths.homeDir + "/Downloads";
    paths.appDataDir = paths.configDir + "/NexusForge";
    
    return paths;
}

std::string PlatformAbstraction::getPlatformName() {
    return "Linux";
}

bool PlatformAbstraction::fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool PlatformAbstraction::directoryExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool PlatformAbstraction::createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0 || directoryExists(path);
}

bool PlatformAbstraction::deleteFile(const std::string& path) {
    return unlink(path.c_str()) == 0;
}

bool PlatformAbstraction::deleteDirectory(const std::string& path) {
    return rmdir(path.c_str()) == 0;
}

bool PlatformAbstraction::copyFile(const std::string& source, const std::string& dest) {
    std::ifstream src(source, std::ios::binary);
    if (!src.is_open()) return false;
    std::ofstream dst(dest, std::ios::binary);
    if (!dst.is_open()) return false;
    dst << src.rdbuf();
    return dst.good();
}

bool PlatformAbstraction::moveFile(const std::string& source, const std::string& dest) {
    return rename(source.c_str(), dest.c_str()) == 0;
}

std::vector<FileInfo> PlatformAbstraction::listDirectory(const std::string& path) {
    std::vector<FileInfo> files;
    DIR* dir = opendir(path.c_str());
    if (!dir) return files;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        
        FileInfo info;
        info.path = path + "/" + name;
        info.name = name;
        
        struct stat st;
        if (stat(info.path.c_str(), &st) == 0) {
            info.isDirectory = S_ISDIR(st.st_mode);
            info.isFile = S_ISREG(st.st_mode);
            info.size = st.st_size;
            info.createdTime = st.st_ctime;
            info.modifiedTime = st.st_mtime;
            info.accessedTime = st.st_atime;
            
            size_t dotPos = name.rfind('.');
            if (dotPos != std::string::npos) {
                info.extension = name.substr(dotPos);
            }
        }
        
        files.push_back(info);
    }
    closedir(dir);
    return files;
}

std::string PlatformAbstraction::getCurrentWorkingDirectory() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        return cwd;
    }
    return ".";
}

bool PlatformAbstraction::setCurrentWorkingDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

std::string PlatformAbstraction::getAbsolutePath(const std::string& relativePath) {
    char* abs = realpath(relativePath.c_str(), nullptr);
    if (abs) {
        std::string result(abs);
        free(abs);
        return result;
    }
    return relativePath;
}

std::string PlatformAbstraction::joinPath(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    if (path2[0] == '/') return path2;
    return path1 + "/" + path2;
}

std::string PlatformAbstraction::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool PlatformAbstraction::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    return true;
}

std::vector<char> PlatformAbstraction::readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    
    std::streampos pos = file.tellg();
    if (pos <= 0) return {};
    std::streamsize size = static_cast<std::streamsize>(pos);
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(static_cast<size_t>(size));
    if (!file.read(buffer.data(), size)) {
        buffer.resize(static_cast<size_t>(file.gcount()));
    }
    return buffer;
}

bool PlatformAbstraction::writeBinaryFile(const std::string& path, const std::vector<char>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    if (!data.empty()) {
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
    }
    return file.good();
}

int PlatformAbstraction::getCurrentProcessId() {
    return getpid();
}

ProcessInfo PlatformAbstraction::getCurrentProcessInfo() {
    ProcessInfo info;
    info.pid = getpid();
    
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        info.path = path;
        size_t slashPos = info.path.rfind('/');
        info.name = (slashPos != std::string::npos) ? info.path.substr(slashPos + 1) : info.path;
    }
    
    return info;
}

bool PlatformAbstraction::openExternal(const std::string& url) {
    return system(("xdg-open \"" + url + "\"").c_str()) == 0;
}

bool PlatformAbstraction::openFileWithDefaultApp(const std::string& path) {
    return openExternal(path);
}

std::string PlatformAbstraction::executeCommand(const std::string& command, int* exitCode) {
    std::array<char, 128> buffer;
    std::string result;
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    int status = pclose(pipe);
    if (exitCode) {
        *exitCode = WEXITSTATUS(status);
    }
    
    return result;
}

std::string PlatformAbstraction::getEnvironmentVariable(const std::string& name) {
    const char* value = getenv(name.c_str());
    return value ? value : "";
}

bool PlatformAbstraction::setEnvironmentVariable(const std::string& name, const std::string& value) {
    return setenv(name.c_str(), value.c_str(), 1) == 0;
}

std::vector<std::string> PlatformAbstraction::getCommandLineArguments() {
    // Implementation would read /proc/self/cmdline
    return {};
}

std::string PlatformAbstraction::getClipboardText() {
    // Would use X11/Wayland clipboard API
    return "";
}

void PlatformAbstraction::setClipboardText(const std::string& text) {
    // Would use X11/Wayland clipboard API
}

bool PlatformAbstraction::hasClipboardText() {
    return !getClipboardText().empty();
}

uint64_t PlatformAbstraction::getCurrentTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

uint64_t PlatformAbstraction::getCurrentTimeMicros() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void PlatformAbstraction::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void PlatformAbstraction::processEvents() {
    auto& state = getDisplayState();
    std::vector<PlatformEvent> dispatched;
    WindowEventCallback callback;

    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (!state.display || !state.x11.loaded) return;
        callback = state.eventCallback;

        while (state.x11.fnXPending(state.display) > 0) {
            X11Event ev{};
            state.x11.fnXNextEvent(state.display, &ev);

            LinuxWindow* win = state.findByXWindow(ev.xany.window);
            if (!win) continue;

            switch (ev.type) {
                case X11_ClientMessage: {
                    if (static_cast<X11Atom>(ev.xclient.data.l[0]) == state.wmDeleteWindow) {
                        win->closeRequested = true;
                        PlatformEvent pe;
                        pe.type = PlatformEventType::WindowClose;
                        pe.window = win;
                        dispatched.push_back(pe);
                    }
                    break;
                }
                case X11_ConfigureNotify: {
                    int nw = std::max(1, ev.xconfigure.width);
                    int nh = std::max(1, ev.xconfigure.height);
                    if (nw != win->width || nh != win->height) {
                        win->width = nw;
                        win->height = nh;
                        PlatformEvent pe;
                        pe.type = PlatformEventType::WindowResize;
                        pe.window = win;
                        pe.width = nw;
                        pe.height = nh;
                        dispatched.push_back(pe);
                    }
                    break;
                }
                case X11_Expose: {
                    if (ev.xexpose.count == 0) {
                        PlatformEvent pe;
                        pe.type = PlatformEventType::WindowExpose;
                        pe.window = win;
                        pe.width = win->width;
                        pe.height = win->height;
                        dispatched.push_back(pe);
                    }
                    break;
                }
                case X11_KeyPress: {
                    char buf[32] = {};
                    X11KeySym ks = 0;
                    int len = state.x11.fnXLookupString(&ev.xkey, buf, static_cast<int>(sizeof(buf) - 1), &ks, nullptr);
                    if (ks == 0) {
                        ks = state.x11.fnXLookupKeysym(&ev.xkey, 0);
                    }
                    int mods = translateModifiers(ev.xkey.state);
                    int keyCode = translateKeySym(ks);

                    PlatformEvent keyEv;
                    keyEv.type = PlatformEventType::KeyPress;
                    keyEv.window = win;
                    keyEv.keyCode = keyCode;
                    keyEv.scanCode = static_cast<int>(ev.xkey.keycode);
                    keyEv.modifiers = mods;
                    dispatched.push_back(keyEv);

                    // Emit CharInput for printable ASCII characters (when Ctrl/Alt not held)
                    if ((mods & 0x0006) == 0 && len > 0) {
                        unsigned char ch = static_cast<unsigned char>(buf[0]);
                        if (ch >= 32 && ch < 127) {
                            PlatformEvent charEv;
                            charEv.type = PlatformEventType::CharInput;
                            charEv.window = win;
                            charEv.codepoint = static_cast<uint32_t>(ch);
                            charEv.modifiers = mods;
                            dispatched.push_back(charEv);
                        }
                    }
                    break;
                }
                case X11_KeyRelease: {
                    X11KeySym ks = state.x11.fnXLookupKeysym(&ev.xkey, 0);
                    PlatformEvent keyEv;
                    keyEv.type = PlatformEventType::KeyRelease;
                    keyEv.window = win;
                    keyEv.keyCode = translateKeySym(ks);
                    keyEv.scanCode = static_cast<int>(ev.xkey.keycode);
                    keyEv.modifiers = translateModifiers(ev.xkey.state);
                    dispatched.push_back(keyEv);
                    break;
                }
                case X11_MotionNotify: {
                    PlatformEvent pe;
                    pe.type = PlatformEventType::MouseMove;
                    pe.window = win;
                    pe.x = static_cast<double>(ev.xmotion.x);
                    pe.y = static_cast<double>(ev.xmotion.y);
                    pe.modifiers = translateModifiers(ev.xmotion.state);
                    dispatched.push_back(pe);
                    break;
                }
                case X11_ButtonPress: {
                    if (ev.xbutton.button == 4 || ev.xbutton.button == 5) {
                        PlatformEvent pe;
                        pe.type = PlatformEventType::MouseScroll;
                        pe.window = win;
                        pe.x = static_cast<double>(ev.xbutton.x);
                        pe.y = static_cast<double>(ev.xbutton.y);
                        pe.scrollY = (ev.xbutton.button == 4) ? 3.0 : -3.0;
                        dispatched.push_back(pe);
                    } else {
                        PlatformEvent pe;
                        pe.type = PlatformEventType::MouseButtonPress;
                        pe.window = win;
                        pe.button = static_cast<int>(ev.xbutton.button) - 1; // 0=left, 1=middle, 2=right
                        pe.x = static_cast<double>(ev.xbutton.x);
                        pe.y = static_cast<double>(ev.xbutton.y);
                        pe.modifiers = translateModifiers(ev.xbutton.state);
                        dispatched.push_back(pe);
                    }
                    break;
                }
                case X11_ButtonRelease: {
                    if (ev.xbutton.button != 4 && ev.xbutton.button != 5) {
                        PlatformEvent pe;
                        pe.type = PlatformEventType::MouseButtonRelease;
                        pe.window = win;
                        pe.button = static_cast<int>(ev.xbutton.button) - 1;
                        pe.x = static_cast<double>(ev.xbutton.x);
                        pe.y = static_cast<double>(ev.xbutton.y);
                        pe.modifiers = translateModifiers(ev.xbutton.state);
                        dispatched.push_back(pe);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (callback) {
        for (const auto& pe : dispatched) {
            callback(pe);
        }
    }
}

void PlatformAbstraction::pumpEvents() {
    processEvents();
}

std::string PlatformAbstraction::openFileDialog(const std::string& title,
                                                  const std::vector<std::string>& filters,
                                                  const std::string& defaultPath) {
    // Would use zenity or kdialog
    std::string cmd = "zenity --file-selection --title=\"" + title + "\"";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    return executeCommand(cmd);
}

std::string PlatformAbstraction::saveFileDialog(const std::string& title,
                                                  const std::vector<std::string>& filters,
                                                  const std::string& defaultPath) {
    std::string cmd = "zenity --file-selection --save --title=\"" + title + "\"";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    return executeCommand(cmd);
}

std::string PlatformAbstraction::selectDirectoryDialog(const std::string& title,
                                                        const std::string& defaultPath) {
    std::string cmd = "zenity --file-selection --directory --title=\"" + title + "\"";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    return executeCommand(cmd);
}

PlatformAbstraction::MessageBoxResult PlatformAbstraction::showMessageBox(
    const std::string& title,
    const std::string& message,
    MessageBoxType type,
    MessageBoxButton buttons) {
    
    std::string cmd = "zenity --";
    switch (type) {
        case MessageBoxType::Info: cmd += "info"; break;
        case MessageBoxType::Warning: cmd += "warning"; break;
        case MessageBoxType::Error: cmd += "error"; break;
        case MessageBoxType::Question: cmd += "question"; break;
    }
    cmd += " --title=\"" + title + "\" --text=\"" + message + "\"";
    
    int exitCode;
    executeCommand(cmd, &exitCode);
    
    if (exitCode == 0) return MessageBoxResult::OK;
    if (exitCode == 1) return MessageBoxResult::Cancel;
    return MessageBoxResult::Cancel;
}

void PlatformAbstraction::setThreadName(const std::string& name) {
    // pthread_setname_np not portable, skipping
}

int PlatformAbstraction::getCurrentThreadId() {
    return gettid();
}

std::string PlatformAbstraction::getHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return hostname;
    }
    return "localhost";
}

std::string PlatformAbstraction::getUserName() {
    const char* user = getenv("USER");
    if (!user) {
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_name) {
            user = pw->pw_name;
        }
    }
    return user ? user : "unknown";
}

bool PlatformAbstraction::isNetworkAvailable() {
    return system("ping -c1 8.8.8.8 > /dev/null 2>&1") == 0;
}

// Native Linux X11 window management (with graceful headless fallback)
static LinuxWindow* createLinuxWindowImpl(const std::string& title, int width, int height, bool splash) {
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = new LinuxWindow();
    win->width = std::max(1, width);
    win->height = std::max(1, height);
    win->isSplash = splash;

    if (!state.display) {
        state.initDisplay();
    }

    if (state.display && state.x11.loaded) {
        X11Window root = state.x11.fnXRootWindow(state.display, state.screen);
        int screenW = state.x11.fnXDisplayWidth(state.display, state.screen);
        int screenH = state.x11.fnXDisplayHeight(state.display, state.screen);
        int posX = std::max(0, (screenW - win->width) / 2);
        int posY = std::max(0, (screenH - win->height) / 2);
        unsigned long black = state.x11.fnXBlackPixel(state.display, state.screen);

        win->xwin = state.x11.fnXCreateSimpleWindow(
            state.display, root, posX, posY,
            static_cast<unsigned int>(win->width),
            static_cast<unsigned int>(win->height),
            0, black, black);

        if (win->xwin) {
            win->hasNativeX11 = true;
            state.x11.fnXSelectInput(state.display, win->xwin, X11_EventMask);
            state.x11.fnXStoreName(state.display, win->xwin, title.c_str());

            if (state.netWmName && state.utf8String) {
                state.x11.fnXChangeProperty(
                    state.display, win->xwin, state.netWmName, state.utf8String, 8,
                    X11_PropModeReplace,
                    reinterpret_cast<const unsigned char*>(title.c_str()),
                    static_cast<int>(title.size()));
            }

            if (state.wmDeleteWindow) {
                state.x11.fnXSetWMProtocols(state.display, win->xwin, &state.wmDeleteWindow, 1);
            }

            if (splash) {
                // Request borderless, centered splash window from EWMH window managers
                if (state.netWmWindowType && state.netWmWindowTypeSplash) {
                    state.x11.fnXChangeProperty(
                        state.display, win->xwin, state.netWmWindowType, X11_XA_ATOM, 32,
                        X11_PropModeReplace,
                        reinterpret_cast<const unsigned char*>(&state.netWmWindowTypeSplash), 1);
                }
                // Motif borderless fallback for WMs that ignore _NET_WM_WINDOW_TYPE_SPLASH
                if (state.motifWmHints) {
                    struct MotifHints {
                        unsigned long flags;
                        unsigned long functions;
                        unsigned long decorations;
                        long inputMode;
                        unsigned long status;
                    } hints = {2UL, 0UL, 0UL, 0L, 0UL};
                    state.x11.fnXChangeProperty(
                        state.display, win->xwin, state.motifWmHints, state.motifWmHints, 32,
                        X11_PropModeReplace,
                        reinterpret_cast<const unsigned char*>(&hints), 5);
                }
            }

            // Size & center position hints
            if (state.x11.fnXAllocSizeHints && state.x11.fnXSetWMNormalHints && state.x11.fnXFree) {
                X11SizeHints* sh = state.x11.fnXAllocSizeHints();
                if (sh) {
                    sh->x = posX;
                    sh->y = posY;
                    sh->width = win->width;
                    sh->height = win->height;
                    if (splash) {
                        sh->flags = X11_USPosition | X11_PPosition | X11_PSize | X11_PMinSize | X11_PMaxSize;
                        sh->min_width = sh->max_width = win->width;
                        sh->min_height = sh->max_height = win->height;
                    } else {
                        sh->flags = X11_USPosition | X11_PPosition | X11_PSize | X11_PMinSize;
                        sh->min_width = 720;
                        sh->min_height = 480;
                    }
                    state.x11.fnXSetWMNormalHints(state.display, win->xwin, sh);
                    state.x11.fnXFree(sh);
                }
            }

            win->gc = state.x11.fnXCreateGC(state.display, win->xwin, 0, nullptr);
            state.x11.fnXFlush(state.display);
        }
    }

    state.windows.push_back(win);
    return win;
}

void* PlatformAbstraction::createMainWindow(const std::string& title, int width, int height) {
    return createLinuxWindowImpl(title, width, height, false);
}

void* PlatformAbstraction::createSplashWindow(int width, int height) {
    return createLinuxWindowImpl("NexusForge IDE", width, height, true);
}

void PlatformAbstraction::destroyWindow(void* window) {
    if (!window) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto it = std::find(state.windows.begin(), state.windows.end(), static_cast<LinuxWindow*>(window));
    if (it == state.windows.end()) return;

    LinuxWindow* win = *it;
    state.windows.erase(it);

    state.destroyImage(win);
    if (state.display && win->hasNativeX11) {
        if (win->gc) state.x11.fnXFreeGC(state.display, win->gc);
        if (win->xwin) state.x11.fnXDestroyWindow(state.display, win->xwin);
        state.x11.fnXFlush(state.display);
    }
    delete win;
}

void PlatformAbstraction::showWindow(void* window) {
    if (!window) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    win->visible = true;
    if (state.display && win->hasNativeX11) {
        int screenW = state.x11.fnXDisplayWidth(state.display, state.screen);
        int screenH = state.x11.fnXDisplayHeight(state.display, state.screen);
        int posX = std::max(0, (screenW - win->width) / 2);
        int posY = std::max(0, (screenH - win->height) / 2);
        state.x11.fnXMapRaised(state.display, win->xwin);
        state.x11.fnXMoveWindow(state.display, win->xwin, posX, posY);
        state.x11.fnXSync(state.display, 0);
    }
}

void PlatformAbstraction::hideWindow(void* window) {
    if (!window) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    win->visible = false;
    if (state.display && win->hasNativeX11) {
        state.x11.fnXUnmapWindow(state.display, win->xwin);
        state.x11.fnXFlush(state.display);
    }
}

void PlatformAbstraction::setWindowTitle(void* window, const char* title) {
    if (!window || !title) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    if (state.display && win->hasNativeX11) {
        state.x11.fnXStoreName(state.display, win->xwin, title);
        if (state.netWmName && state.utf8String) {
            state.x11.fnXChangeProperty(
                state.display, win->xwin, state.netWmName, state.utf8String, 8,
                X11_PropModeReplace,
                reinterpret_cast<const unsigned char*>(title),
                static_cast<int>(std::strlen(title)));
        }
        state.x11.fnXFlush(state.display);
    }
}

void PlatformAbstraction::setWindowSize(void* window, int width, int height) {
    if (!window) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    win->width = std::max(1, width);
    win->height = std::max(1, height);
    if (state.display && win->hasNativeX11) {
        state.x11.fnXResizeWindow(state.display, win->xwin,
                                  static_cast<unsigned int>(win->width),
                                  static_cast<unsigned int>(win->height));
        state.x11.fnXFlush(state.display);
    }
}

void PlatformAbstraction::setWindowPosition(void* window, int x, int y) {
    if (!window) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    if (state.display && win->hasNativeX11) {
        state.x11.fnXMoveWindow(state.display, win->xwin, x, y);
        state.x11.fnXFlush(state.display);
    }
}

void PlatformAbstraction::maximizeWindow(void* window) {
    if (!window) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    state.sendNetWmState(win, 1, state.netWmStateMaxHorz, state.netWmStateMaxVert);
}

void PlatformAbstraction::minimizeWindow(void* window) {
    if (!window) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    if (state.display && win->hasNativeX11) {
        state.x11.fnXIconifyWindow(state.display, win->xwin, state.screen);
        state.x11.fnXFlush(state.display);
    }
}

void PlatformAbstraction::restoreWindow(void* window) {
    if (!window) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    win->fullscreen = false;
    state.sendNetWmState(win, 0, state.netWmStateFullscreen);
    state.sendNetWmState(win, 0, state.netWmStateMaxHorz, state.netWmStateMaxVert);
}

void PlatformAbstraction::setWindowFullscreen(void* window, bool fullscreen) {
    if (!window) return;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    win->fullscreen = fullscreen;
    state.sendNetWmState(win, fullscreen ? 1 : 0, state.netWmStateFullscreen);
}

bool PlatformAbstraction::isWindowFullscreen(void* window) {
    if (!window) return false;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return static_cast<LinuxWindow*>(window)->fullscreen;
}

void* PlatformAbstraction::getNativeWindowHandle(void* window) {
    if (!window) return nullptr;
    auto* win = static_cast<LinuxWindow*>(window);
    return win->hasNativeX11 ? reinterpret_cast<void*>(static_cast<uintptr_t>(win->xwin)) : window;
}

bool PlatformAbstraction::hasGraphicalDisplay() {
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.display) {
        state.initDisplay();
    }
    return state.display != nullptr;
}

bool PlatformAbstraction::presentFramebuffer(void* window, const uint32_t* bgraPixels, int width, int height) {
    if (!window || !bgraPixels || width <= 0 || height <= 0) return false;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);

    auto* win = static_cast<LinuxWindow*>(window);
    if (!state.display || !win->hasNativeX11 || !win->gc) return false;

    if (!win->ximage || win->imageWidth != width || win->imageHeight != height) {
        state.destroyImage(win);
        size_t bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * sizeof(uint32_t);
        auto* buf = static_cast<uint32_t*>(std::malloc(bytes));
        if (!buf) return false;
        win->ximage = state.x11.fnXCreateImage(
            state.display, state.visual,
            static_cast<unsigned int>(state.depth),
            X11_ZPixmap, 0,
            reinterpret_cast<char*>(buf),
            static_cast<unsigned int>(width),
            static_cast<unsigned int>(height),
            32, width * 4);
        if (!win->ximage) {
            std::free(buf);
            return false;
        }
        win->ximagePixels = buf;
        win->imageWidth = width;
        win->imageHeight = height;
    }

    size_t bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * sizeof(uint32_t);
    std::memcpy(win->ximagePixels, bgraPixels, bytes);
    state.x11.fnXPutImage(
        state.display, win->xwin, win->gc, win->ximage,
        0, 0, 0, 0,
        static_cast<unsigned int>(width),
        static_cast<unsigned int>(height));
    state.x11.fnXFlush(state.display);
    return true;
}

void PlatformAbstraction::getWindowSize(void* window, int& width, int& height) {
    if (!window) {
        width = 1280;
        height = 720;
        return;
    }
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);
    auto* win = static_cast<LinuxWindow*>(window);
    width = win->width;
    height = win->height;
}

bool PlatformAbstraction::isWindowCloseRequested(void* window) {
    if (!window) return false;
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return static_cast<LinuxWindow*>(window)->closeRequested;
}

void PlatformAbstraction::setWindowEventCallback(WindowEventCallback callback) {
    auto& state = getDisplayState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.eventCallback = std::move(callback);
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_LINUX
