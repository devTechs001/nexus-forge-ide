# CMakeLists.txt - NexusForge IDE Build Configuration
cmake_minimum_required(VERSION 3.20)

project(NexusForge
    VERSION 1.0.0
    DESCRIPTION "Cross-Platform AI-Powered IDE"
    LANGUAGES C CXX ASM
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
option(NEXUS_BUILD_TESTS "Build unit tests" ON)
option(NEXUS_BUILD_DOCS "Build documentation" OFF)
option(NEXUS_ENABLE_VULKAN "Enable Vulkan renderer" ON)
option(NEXUS_ENABLE_OPENGL "Enable OpenGL renderer" ON)
option(NEXUS_ENABLE_METAL "Enable Metal renderer (macOS/iOS)" ON)
option(NEXUS_ENABLE_DX12 "Enable DirectX 12 renderer (Windows)" ON)
option(NEXUS_ENABLE_AI "Enable AI features" ON)
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

if(NEXUS_ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
endif()

if(NEXUS_ENABLE_OPENGL)
    find_package(OpenGL REQUIRED)
endif()

# Third-party dependencies
include(FetchContent)

# GLFW for windowing
if(NOT NEXUS_PLATFORM_ANDROID AND NOT NEXUS_PLATFORM_IOS AND NOT NEXUS_PLATFORM_WEB)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

# FreeType for font rendering
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-0
)
FetchContent_MakeAvailable(freetype)

# Tree-sitter for syntax highlighting
FetchContent_Declare(
    tree-sitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    GIT_TAG v0.20.8
)
FetchContent_MakeAvailable(tree-sitter)

# JSON library
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Compression (for extensions)
FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.2.13
)
FetchContent_MakeAvailable(zlib)

# SSL/TLS
find_package(OpenSSL REQUIRED)

# Source files
set(NEXUS_CORE_SOURCES
    core/engine/nexus_core.cpp
    core/engine/memory_manager.cpp
    core/engine/thread_pool.cpp
    core/engine/event_system.cpp
    core/engine/plugin_loader.cpp
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
    editor/syntax/tree_sitter_bridge.cpp
    editor/syntax/token_colorizer.cpp
    editor/intellisense/lsp_client.cpp
    editor/intellisense/completion_provider.cpp
    editor/intellisense/signature_help.cpp
    editor/features/multi_cursor.cpp
    editor/features/code_folding.cpp
    editor/features/bracket_matching.cpp
    editor/features/search_replace.cpp
    editor/features/git_integration.cpp
)

set(NEXUS_AI_SOURCES
    ai/models/model_manager.cpp
    ai/models/onnx_runtime.cpp
    ai/features/ai_autocomplete.cpp
    ai/features/ai_chat.cpp
    ai/features/ai_refactor.cpp
    ai/features/ai_explain.cpp
    ai/providers/openai_provider.cpp
    ai/providers/anthropic_provider.cpp
    ai/providers/local_llm_provider.cpp
)

if(NEXUS_ENABLE_LLAMA)
    list(APPEND NEXUS_AI_SOURCES ai/models/llama_inference.cpp)
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
    debugger/debug_adapter_protocol.cpp
    debugger/breakpoint_manager.cpp
    debugger/variable_inspector.cpp
)

set(NEXUS_TERMINAL_SOURCES
    terminal/terminal_emulator.cpp
    terminal/pty_handler.cpp
    terminal/shell_integration.cpp
)

set(NEXUS_NETWORK_SOURCES
    networking/http_client.cpp
    networking/websocket_client.cpp
    networking/live_share.cpp
    networking/remote_dev.cpp
)

# Assembly sources (x86_64)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    enable_language(ASM_NASM)
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

    set(NEXUS_ASM_SOURCES
        core/asm/simd_operations.asm
        core/asm/crypto_accel.asm
        core/asm/memory_ops.asm
    )
endif()

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
    ${NEXUS_ASM_SOURCES}
)

# Include directories
target_include_directories(NexusForge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/editor
    ${CMAKE_CURRENT_SOURCE_DIR}/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/extensions
    ${Vulkan_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(NexusForge PRIVATE
    Threads::Threads
    freetype
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    zlibstatic
)

if(NOT NEXUS_PLATFORM_ANDROID AND NOT NEXUS_PLATFORM_IOS AND NOT NEXUS_PLATFORM_WEB)
    target_link_libraries(NexusForge PRIVATE glfw)
endif()

if(NEXUS_ENABLE_VULKAN)
    target_link_libraries(NexusForge PRIVATE Vulkan::Vulkan)
endif()

if(NEXUS_ENABLE_OPENGL)
    target_link_libraries(NexusForge PRIVATE OpenGL::GL)
endif()

if(NEXUS_ENABLE_ONNX)
    target_link_libraries(NexusForge PRIVATE onnxruntime)
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
        pthread
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
elseif(NEXUS_PLATFORM_ANDROID)
    target_link_libraries(NexusForge PRIVATE
        android
        log
        EGL
        GLESv3
    )
endif()

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(NexusForge PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        $<$<CONFIG:Release>:-O3 -march=native>
        $<$<CONFIG:Debug>:-g -O0 -fsanitize=address,undefined>
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(NexusForge PRIVATE -Wno-gnu-anonymous-struct)
    endif()
elseif(MSVC)
    target_compile_options(NexusForge PRIVATE
        /W4 /WX-
        /permissive-
        /Zc:__cplusplus
        $<$<CONFIG:Release>:/O2 /GL>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
endif()

# Resources
if(NEXUS_PLATFORM_WINDOWS)
    target_sources(NexusForge PRIVATE resources/nexusforge.rc)
endif()

# Installation
install(TARGETS NexusForge
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY resources/
    DESTINATION share/nexusforge
)

# Tests
if(NEXUS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
if(NEXUS_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
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
