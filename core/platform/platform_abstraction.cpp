// core/platform/platform_abstraction.cpp
#include "platform_abstraction.hpp"

namespace NexusForge::Platform {

// This file provides stub implementations that link to platform-specific files

PlatformType getCurrentPlatform() {
#if defined(NEXUS_PLATFORM_WINDOWS)
    return PlatformType::Windows;
#elif defined(NEXUS_PLATFORM_LINUX)
    return PlatformType::Linux;
#elif defined(NEXUS_PLATFORM_MACOS)
    return PlatformType::macOS;
#elif defined(NEXUS_PLATFORM_ANDROID)
    return PlatformType::Android;
#elif defined(NEXUS_PLATFORM_IOS)
    return PlatformType::iOS;
#elif defined(NEXUS_PLATFORM_WEB)
    return PlatformType::Web;
#else
    return PlatformType::Linux;  // Default
#endif
}

std::string getPlatformName() {
    switch (getCurrentPlatform()) {
        case PlatformType::Windows: return "Windows";
        case PlatformType::Linux: return "Linux";
        case PlatformType::macOS: return "macOS";
        case PlatformType::Android: return "Android";
        case PlatformType::iOS: return "iOS";
        case PlatformType::Web: return "Web";
        default: return "Unknown";
    }
}

bool isPlatform(PlatformType platform) {
    return getCurrentPlatform() == platform;
}

bool initialize() {
    // Platform-specific initialization handled in platform_*.cpp
    return true;
}

void shutdown() {
    // Platform-specific cleanup handled in platform_*.cpp
}

void processEvents() {
    // Platform-specific event processing
}

void pumpEvents() {
    processEvents();
}

} // namespace NexusForge::Platform
