# CMakeLists.txt - NexusForge IDE Build Configuration
cmake_minimum_required(VERSION 3.20)

project(NexusForge
    VERSION 1.0.0
    DESCRIPTION "Cross-Platform AI-Powered IDE"
    LANGUAGES C CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Options
option(NEXUS_BUILD_TESTS "Build unit tests" OFF)
option(NEXUS_BUILD_DOCS "Build documentation" OFF)
option(NEXUS_ENABLE_VULKAN "Enable Vulkan renderer" OFF)
option(NEXUS_ENABLE_OPENGL "Enable OpenGL renderer" OFF)
option(NEXUS_ENABLE_METAL "Enable Metal renderer (macOS/iOS)" ON)
option(NEXUS_ENABLE_DX12 "Enable DirectX 12 renderer (Windows)" ON)
option(NEXUS_ENABLE_AI "Enable AI features" OFF)
option(NEXUS_ENABLE_ONNX "Enable ONNX Runtime" ON)
option(NEXUS_ENABLE_LLAMA "Enable local LLM support" ON)
option(NEXUS_ENABLE_WEBVIEW "Enable webview support" ON)
option(NEXUS_ENABLE_TELEMETRY "Enable telemetry" OFF)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(NEXUS_PLATFORM_WINDOWS TRUE)
    add_definitions(-DNEXUS_PLATFORM_WINDOWS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(NEXUS_PLATFORM_LINUX TRUE)
    add_definitions(-DNEXUS_PLATFORM_LINUX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(NEXUS_PLATFORM_MACOS TRUE)
    add_definitions(-DNEXUS_PLATFORM_MACOS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(NEXUS_PLATFORM_ANDROID TRUE)
    add_definitions(-DNEXUS_PLATFORM_ANDROID)
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(NEXUS_PLATFORM_IOS TRUE)
    add_definitions(-DNEXUS_PLATFORM_IOS)
elseif(EMSCRIPTEN)
    set(NEXUS_PLATFORM_WEB TRUE)
    add_definitions(-DNEXUS_PLATFORM_WEB)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find packages
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Freetype REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(nlohmann_json REQUIRED)

# TreeSitter - use pkg-config or find library directly
find_package(PkgConfig REQUIRED)
pkg_check_modules(TREESITTER QUIET tree-sitter)
if(TREESITTER_FOUND)
    set(TreeSitter_INCLUDE_DIRS ${TREESITTER_INCLUDE_DIRS})
    set(TreeSitter_LIBRARIES ${TREESITTER_LIBRARIES})
else()
    # Fallback: try to find tree-sitter directly
    find_path(TreeSitter_INCLUDE_DIRS tree_sitter/api.h PATH_SUFFIXES tree_sitter)
    find_library(TreeSitter_LIBRARIES NAMES tree-sitter)
endif()

# Tree-sitter grammars (optional - would need to be built separately)
# find_package(TreeSitterCpp QUIET)
# find_package(TreeSitterPython QUIET)

# Source files
set(NEXUS_CORE_SOURCES
    core/engine/nexus_core.cpp
    core/engine/memory_manager.cpp
    core/engine/thread_pool.cpp
    core/engine/event_system.cpp
    core/engine/plugin_loader.cpp
    core/engine/extension_manager.cpp
    core/engine/workspace_manager.cpp
    core/platform/platform_abstraction.cpp
)

if(NEXUS_PLATFORM_WINDOWS)
    list(APPEND NEXUS_CORE_SOURCES core/platform/platform_windows.cpp)
elseif(NEXUS_PLATFORM_LINUX)
    list(APPEND NEXUS_CORE_SOURCES core/platform/platform_linux.cpp)
elseif(NEXUS_PLATFORM_MACOS)
    list(APPEND NEXUS_CORE_SOURCES core/platform/platform_macos.cpp)
elseif(NEXUS_PLATFORM_ANDROID)
    list(APPEND NEXUS_CORE_SOURCES core/platform/platform_android.cpp)
elseif(NEXUS_PLATFORM_IOS)
    list(APPEND NEXUS_CORE_SOURCES core/platform/platform_ios.cpp)
elseif(NEXUS_PLATFORM_WEB)
    list(APPEND NEXUS_CORE_SOURCES core/platform/platform_web.cpp)
endif()

set(NEXUS_UI_SOURCES
    ui/framework/ui_core.cpp
    ui/framework/widget_system.cpp
    ui/framework/layout_engine.cpp
    ui/framework/animation_engine.cpp
    ui/framework/theme_manager.cpp
    ui/renderer/render_engine.cpp
    ui/renderer/font_renderer.cpp
    ui/renderer/texture_manager.cpp
    ui/renderer/shader_manager.cpp
    ui/widgets/button.cpp
    ui/widgets/text_input.cpp
    ui/widgets/code_editor.cpp
    ui/widgets/tree_view.cpp
    ui/widgets/tab_bar.cpp
    ui/widgets/terminal.cpp
    ui/widgets/status_bar.cpp
    ui/widgets/command_palette.cpp
    ui/widgets/minimap.cpp
    ui/screens/splash_screen.cpp
    ui/screens/main_window.cpp
    ui/screens/settings_screen.cpp
    ui/screens/welcome_screen.cpp
    ui/screens/onboarding_screen.cpp
    ui/mobile/touch_handler.cpp
    ui/mobile/gesture_recognizer.cpp
    ui/mobile/adaptive_layout.cpp
    ui/mobile/mobile_toolbar.cpp
)

if(NEXUS_ENABLE_VULKAN)
    list(APPEND NEXUS_UI_SOURCES ui/renderer/vulkan_backend.cpp)
endif()
if(NEXUS_ENABLE_OPENGL)
    list(APPEND NEXUS_UI_SOURCES ui/renderer/opengl_backend.cpp)
endif()
if(NEXUS_ENABLE_METAL AND (NEXUS_PLATFORM_MACOS OR NEXUS_PLATFORM_IOS))
    list(APPEND NEXUS_UI_SOURCES ui/renderer/metal_backend.mm)
endif()
if(NEXUS_ENABLE_DX12 AND NEXUS_PLATFORM_WINDOWS)
    list(APPEND NEXUS_UI_SOURCES ui/renderer/directx_backend.cpp)
endif()

set(NEXUS_EDITOR_SOURCES
    editor/buffer/text_buffer.cpp
    editor/buffer/gap_buffer.cpp
    editor/buffer/rope_buffer.cpp
    editor/buffer/piece_table.cpp
    editor/buffer/undo_manager.cpp
    editor/syntax/syntax_highlighter.cpp
)

set(NEXUS_AI_SOURCES
    ai/models/model_manager.cpp
)

if(NEXUS_ENABLE_AI)
    list(APPEND NEXUS_AI_SOURCES
        ai/features/ai_autocomplete.cpp
        ai/features/ai_chat.cpp
        ai/providers/openai_provider.cpp
        ai/providers/anthropic_provider.cpp
        ai/providers/local_llm_provider.cpp
    )
endif()

set(NEXUS_EXTENSION_SOURCES
    extensions/api/extension_api.cpp
    extensions/api/extension_host.cpp
    extensions/api/sandbox.cpp
    extensions/api/ipc_channel.cpp
    extensions/marketplace/marketplace_client.cpp
    extensions/marketplace/extension_installer.cpp
)

set(NEXUS_WORKSPACE_SOURCES
    workspace/workspace_manager.cpp
    workspace/project_system.cpp
    workspace/file_watcher.cpp
    workspace/search_engine.cpp
)

set(NEXUS_CONFIG_SOURCES
    config/settings_manager.cpp
    config/keybinding_manager.cpp
    config/snippet_manager.cpp
    config/user_data.cpp
)

set(NEXUS_DEBUGGER_SOURCES
    # Temporarily excluded - needs API alignment
)

set(NEXUS_TERMINAL_SOURCES
    # Temporarily excluded - needs API alignment
)

set(NEXUS_NETWORK_SOURCES
    networking/http_client.cpp
    networking/websocket_client.cpp
    networking/live_share.cpp
    networking/remote_dev.cpp
)

# Main executable
add_executable(NexusForge
    main.cpp
    ${NEXUS_CORE_SOURCES}
    ${NEXUS_UI_SOURCES}
    ${NEXUS_EDITOR_SOURCES}
    ${NEXUS_AI_SOURCES}
    ${NEXUS_EXTENSION_SOURCES}
    ${NEXUS_WORKSPACE_SOURCES}
    ${NEXUS_CONFIG_SOURCES}
    ${NEXUS_DEBUGGER_SOURCES}
    ${NEXUS_TERMINAL_SOURCES}
    ${NEXUS_NETWORK_SOURCES}
)

# Include directories
target_include_directories(NexusForge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/editor
    ${CMAKE_CURRENT_SOURCE_DIR}/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/extensions
    ${OPENGL_INCLUDE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${TreeSitter_INCLUDE_DIRS}
)

# Compile definitions
if(NEXUS_ENABLE_VULKAN)
    target_compile_definitions(NexusForge PRIVATE NEXUS_ENABLE_VULKAN)
endif()
if(NEXUS_ENABLE_OPENGL)
    target_compile_definitions(NexusForge PRIVATE NEXUS_ENABLE_OPENGL)
endif()
if(NEXUS_ENABLE_AI)
    target_compile_definitions(NexusForge PRIVATE NEXUS_ENABLE_AI)
endif()

# Link libraries
target_link_libraries(NexusForge PRIVATE
    Threads::Threads
    glfw
    Freetype::Freetype
    ${TreeSitter_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    nlohmann_json::nlohmann_json
)

if(NEXUS_ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(NexusForge PRIVATE Vulkan::Vulkan)
endif()

if(NEXUS_ENABLE_OPENGL)
    target_link_libraries(NexusForge PRIVATE OpenGL::GL)
endif()

if(NEXUS_ENABLE_ONNX)
    find_package(onnxruntime QUIET)
    if(onnxruntime_FOUND)
        target_link_libraries(NexusForge PRIVATE onnxruntime)
    endif()
endif()

# Platform-specific libraries
if(NEXUS_PLATFORM_WINDOWS)
    target_link_libraries(NexusForge PRIVATE
        dwmapi
        d3d12
        dxgi
        d3dcompiler
        windowscodecs
        shlwapi
    )
elseif(NEXUS_PLATFORM_LINUX)
    target_link_libraries(NexusForge PRIVATE
        X11
        Xrandr
        Xi
        Xinerama
        Xcursor
        dl
    )
elseif(NEXUS_PLATFORM_MACOS)
    target_link_libraries(NexusForge PRIVATE
        "-framework Cocoa"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
        "-framework IOKit"
        "-framework CoreFoundation"
    )
endif()

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(NexusForge PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g -O0>
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(NexusForge PRIVATE -Wno-gnu-anonymous-struct)
    endif()
elseif(MSVC)
    target_compile_options(NexusForge PRIVATE
        /W4 /WX-
        /permissive-
        /Zc:__cplusplus
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
endif()

# Installation
install(TARGETS NexusForge
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY resources/
    DESTINATION share/nexusforge
    OPTIONAL
)

# Tests
if(NEXUS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Package
set(CPACK_PACKAGE_NAME "NexusForge")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "NexusForge Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AI-Powered Cross-Platform IDE")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

if(NEXUS_PLATFORM_WINDOWS)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(NEXUS_PLATFORM_MACOS)
    set(CPACK_GENERATOR "DragNDrop;ZIP")
elseif(NEXUS_PLATFORM_LINUX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
endif()

include(CPack)

message(STATUS "NexusForge IDE Build Configuration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  OpenGL: ${NEXUS_ENABLE_OPENGL}")
message(STATUS "  Vulkan: ${NEXUS_ENABLE_VULKAN}")
message(STATUS "  AI Features: ${NEXUS_ENABLE_AI}")
