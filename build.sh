#!/usr/bin/env bash
# build.sh - Cross-platform build script for NexusForge IDE
# Supports CMake (when available) and a direct C++20 compiler fallback with
# parallel, incremental compilation.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Defaults
BUILD_TYPE="Release"
BACKEND="auto"          # auto | cmake | direct
BUILD_DIR="out/build"
JOBS=""
CLEAN=0
CLEAN_ONLY=0
VERBOSE=0
RUN_AFTER_BUILD=1
RUN_CHECK=0
ENABLE_AI=1
ENABLE_OPENGL=0
ENABLE_VULKAN=0
ENABLE_METAL=0
ENABLE_DX12=0
BUILD_TESTS=0
CXX_BIN="${CXX:-}"
RUN_ARGS=()

# Detect host platform
detect_platform() {
    local uname_s
    uname_s="$(uname -s 2>/dev/null || echo Unknown)"
    case "$uname_s" in
        Linux*)               echo "Linux" ;;
        Darwin*)              echo "macOS" ;;
        MINGW*|MSYS*|CYGWIN*) echo "Windows" ;;
        *)                    echo "Unknown" ;;
    esac
}

detect_jobs() {
    if command -v nproc >/dev/null 2>&1; then
        nproc
    elif command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null || echo 4
    else
        echo 4
    fi
}

PLATFORM="$(detect_platform)"
if [[ "$PLATFORM" == "macOS" ]]; then
    ENABLE_METAL=1
elif [[ "$PLATFORM" == "Windows" ]]; then
    ENABLE_DX12=1
fi

usage() {
    cat <<'EOF'
Usage: ./build.sh [command] [options] [-- <app-args...>]

Commands:
  build                 Build NexusForge IDE (default)
  clean                 Remove the build directory and exit
  rebuild               Clean and build from scratch

Options:
  -t, --type <type>     Build type: Release, Debug, RelWithDebInfo (default: Release)
  -b, --backend <name>  Build backend: auto, cmake, direct (default: auto)
  -o, --build-dir <dir> Build output directory (default: out/build)
  -j, --jobs <N>        Parallel compile jobs (default: CPU count)
  -c, --clean           Clean build directory before building
      --compiler <bin>  C++ compiler to use (e.g. g++, clang++)
      --ai / --no-ai    Enable/disable AI features (default: ON)
      --opengl          Enable OpenGL renderer backend
      --vulkan          Enable Vulkan renderer backend
      --metal           Enable Metal renderer backend (macOS)
      --dx12            Enable DirectX renderer backend (Windows)
      --tests           Enable CMake unit tests (NEXUS_BUILD_TESTS=ON)
      --check           Run header syntax check + binary smoke test (--version)
      --run             Run the built NexusForge binary after building (default)
      --no-run          Build only; do not auto-run after building
      --build-only      Alias for --no-run
  -v, --verbose         Show full compiler commands
  -h, --help            Show this help message and exit

Examples:
  ./build.sh                            # Release build and auto-run NexusForge
  ./build.sh --no-run                   # Build only without launching
  ./build.sh -t Debug -j 8              # Debug build with 8 jobs and auto-run
  ./build.sh --no-run --check           # Build + header ODR & --version checks
  ./build.sh -- --version               # Build and run with --version
  ./build.sh clean                      # Clean build output directory
EOF
}

# Parse CLI arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        build)
            shift
            ;;
        clean)
            CLEAN_ONLY=1
            shift
            ;;
        rebuild)
            CLEAN=1
            shift
            ;;
        -t|--type)
            [[ $# -ge 2 ]] || { echo "Error: $1 requires an argument" >&2; exit 1; }
            BUILD_TYPE="$2"
            shift 2
            ;;
        -b|--backend)
            [[ $# -ge 2 ]] || { echo "Error: $1 requires an argument" >&2; exit 1; }
            BACKEND="$2"
            shift 2
            ;;
        -o|--build-dir)
            [[ $# -ge 2 ]] || { echo "Error: $1 requires an argument" >&2; exit 1; }
            BUILD_DIR="$2"
            shift 2
            ;;
        -j|--jobs)
            [[ $# -ge 2 ]] || { echo "Error: $1 requires an argument" >&2; exit 1; }
            JOBS="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        --compiler)
            [[ $# -ge 2 ]] || { echo "Error: $1 requires an argument" >&2; exit 1; }
            CXX_BIN="$2"
            shift 2
            ;;
        --ai)
            ENABLE_AI=1
            shift
            ;;
        --no-ai)
            ENABLE_AI=0
            shift
            ;;
        --opengl)
            ENABLE_OPENGL=1
            shift
            ;;
        --no-opengl)
            ENABLE_OPENGL=0
            shift
            ;;
        --vulkan)
            ENABLE_VULKAN=1
            shift
            ;;
        --no-vulkan)
            ENABLE_VULKAN=0
            shift
            ;;
        --metal)
            ENABLE_METAL=1
            shift
            ;;
        --no-metal)
            ENABLE_METAL=0
            shift
            ;;
        --dx12)
            ENABLE_DX12=1
            shift
            ;;
        --no-dx12)
            ENABLE_DX12=0
            shift
            ;;
        --tests)
            BUILD_TESTS=1
            shift
            ;;
        --check)
            RUN_CHECK=1
            shift
            ;;
        --run)
            RUN_AFTER_BUILD=1
            shift
            ;;
        --no-run|--build-only)
            RUN_AFTER_BUILD=0
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            RUN_ARGS=("$@")
            break
            ;;
        *)
            echo "Error: Unknown option '$1'. Use --help for usage." >&2
            exit 1
            ;;
    esac
done

case "$BUILD_TYPE" in
    Release|Debug|RelWithDebInfo) ;;
    *)
        echo "Error: Invalid build type '$BUILD_TYPE' (expected Release, Debug, or RelWithDebInfo)." >&2
        exit 1
        ;;
esac

case "$BACKEND" in
    auto|cmake|direct) ;;
    *)
        echo "Error: Invalid backend '$BACKEND' (expected auto, cmake, or direct)." >&2
        exit 1
        ;;
esac

if [[ -z "$JOBS" ]]; then
    JOBS="$(detect_jobs)"
fi

# Resolve relative build dir against repository root
if [[ "$BUILD_DIR" != /* ]]; then
    BUILD_DIR="$ROOT_DIR/$BUILD_DIR"
fi

# Guard against accidentally deleting repository root or tracked legacy build/
if [[ "$CLEAN_ONLY" -eq 1 || "$CLEAN" -eq 1 ]]; then
    if [[ "$BUILD_DIR" == "$ROOT_DIR" || "$BUILD_DIR" == "/" ]]; then
        echo "Error: Refusing to clean unsafe directory: $BUILD_DIR" >&2
        exit 1
    fi
    echo "[clean] Removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"
    if [[ "$CLEAN_ONLY" -eq 1 ]]; then
        exit 0
    fi
fi

# Resolve backend
if [[ "$BACKEND" == "auto" ]]; then
    if command -v cmake >/dev/null 2>&1; then
        BACKEND="cmake"
    else
        BACKEND="direct"
    fi
fi

# Resolve compiler for direct mode (or if explicitly requested)
resolve_compiler() {
    if [[ -n "$CXX_BIN" ]]; then
        command -v "$CXX_BIN" >/dev/null 2>&1 || {
            echo "Error: Compiler '$CXX_BIN' not found in PATH." >&2
            exit 1
        }
        return
    fi
    for candidate in g++ clang++ c++; do
        if command -v "$candidate" >/dev/null 2>&1; then
            CXX_BIN="$candidate"
            return
        fi
    done
    echo "Error: No C++20 compiler found (tried g++, clang++, c++)." >&2
    exit 1
}

on_off() {
    [[ "$1" -eq 1 ]] && echo "ON" || echo "OFF"
}

BIN_EXT=""
[[ "$PLATFORM" == "Windows" ]] && BIN_EXT=".exe"
BIN_PATH="$BUILD_DIR/bin/NexusForge${BIN_EXT}"

echo "=========================================="
echo " NexusForge IDE Build"
echo "=========================================="
echo " Platform  : $PLATFORM"
echo " Backend   : $BACKEND"
echo " Type      : $BUILD_TYPE"
echo " Jobs      : $JOBS"
echo " Build Dir : $BUILD_DIR"
echo " AI        : $(on_off "$ENABLE_AI")"
echo " OpenGL    : $(on_off "$ENABLE_OPENGL")"
echo " Vulkan    : $(on_off "$ENABLE_VULKAN")"
echo " Auto-Run  : $(on_off "$RUN_AFTER_BUILD")"
echo "=========================================="

build_with_cmake() {
    command -v cmake >/dev/null 2>&1 || {
        echo "Error: cmake requested but not found in PATH. Use --backend direct." >&2
        exit 1
    }

    local cmake_args=(
        -S "$ROOT_DIR"
        -B "$BUILD_DIR"
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DNEXUS_ENABLE_AI=$(on_off "$ENABLE_AI")"
        "-DNEXUS_ENABLE_OPENGL=$(on_off "$ENABLE_OPENGL")"
        "-DNEXUS_ENABLE_VULKAN=$(on_off "$ENABLE_VULKAN")"
        "-DNEXUS_ENABLE_METAL=$(on_off "$ENABLE_METAL")"
        "-DNEXUS_ENABLE_DX12=$(on_off "$ENABLE_DX12")"
        "-DNEXUS_BUILD_TESTS=$(on_off "$BUILD_TESTS")"
    )

    if [[ -n "$CXX_BIN" ]]; then
        cmake_args+=("-DCMAKE_CXX_COMPILER=$CXX_BIN")
    fi
    if command -v ninja >/dev/null 2>&1; then
        cmake_args+=(-G Ninja)
    fi

    echo "[cmake] Configuring..."
    cmake "${cmake_args[@]}"

    local build_cmd=(cmake --build "$BUILD_DIR" --parallel "$JOBS")
    [[ "$VERBOSE" -eq 1 ]] && build_cmd+=(--verbose)

    echo "[cmake] Building..."
    "${build_cmd[@]}"
}

build_direct() {
    resolve_compiler
    echo "[direct] Compiler: $CXX_BIN ($("$CXX_BIN" --version | head -n 1))"

    local obj_dir="$BUILD_DIR/obj"
    mkdir -p "$obj_dir" "$BUILD_DIR/bin"

    # Build source list matching CMakeLists.txt
    local sources=(
        main.cpp
        # Core
        core/engine/nexus_core.cpp
        core/engine/memory_manager.cpp
        core/engine/thread_pool.cpp
        core/engine/event_system.cpp
        core/engine/plugin_loader.cpp
        core/engine/extension_manager.cpp
        core/engine/workspace_manager.cpp
        core/platform/platform_abstraction.cpp
        # UI Framework & Renderer
        ui/framework/ui_core.cpp
        ui/framework/widget_system.cpp
        ui/framework/layout_engine.cpp
        ui/framework/animation_engine.cpp
        ui/framework/theme_manager.cpp
        ui/renderer/render_engine.cpp
        ui/renderer/software_backend.cpp
        ui/renderer/font_renderer.cpp
        ui/renderer/texture_manager.cpp
        ui/renderer/shader_manager.cpp
        # UI Widgets
        ui/widgets/button.cpp
        ui/widgets/text_input.cpp
        ui/widgets/code_editor.cpp
        ui/widgets/tree_view.cpp
        ui/widgets/tab_bar.cpp
        ui/widgets/terminal.cpp
        ui/widgets/status_bar.cpp
        ui/widgets/command_palette.cpp
        ui/widgets/minimap.cpp
        # UI Screens
        ui/screens/splash_screen.cpp
        ui/screens/main_window.cpp
        ui/screens/settings_screen.cpp
        ui/screens/welcome_screen.cpp
        ui/screens/onboarding_screen.cpp
        # UI Mobile
        ui/mobile/touch_handler.cpp
        ui/mobile/gesture_recognizer.cpp
        ui/mobile/adaptive_layout.cpp
        ui/mobile/mobile_toolbar.cpp
        # Editor
        editor/buffer/text_buffer.cpp
        editor/buffer/gap_buffer.cpp
        editor/buffer/rope_buffer.cpp
        editor/buffer/piece_table.cpp
        editor/buffer/undo_manager.cpp
        editor/syntax/syntax_highlighter.cpp
        editor/syntax/tree_sitter_bridge.cpp
        editor/features/bracket_matching.cpp
        editor/features/code_folding.cpp
        editor/features/git_integration.cpp
        editor/features/multi_cursor.cpp
        editor/features/search_replace.cpp
        editor/intellisense/lsp_client.cpp
        editor/intellisense/completion_provider.cpp
        # AI base
        ai/models/model_manager.cpp
        # Extensions
        extensions/api/extension_api.cpp
        extensions/api/extension_host.cpp
        extensions/api/sandbox.cpp
        extensions/api/ipc_channel.cpp
        extensions/marketplace/marketplace_client.cpp
        extensions/marketplace/extension_installer.cpp
        # Workspace
        workspace/workspace_manager.cpp
        workspace/project_system.cpp
        workspace/file_watcher.cpp
        workspace/search_engine.cpp
        # Config
        config/settings_manager.cpp
        config/keybinding_manager.cpp
        config/snippet_manager.cpp
        config/user_data.cpp
        # Debugger
        debugger/breakpoint_manager.cpp
        debugger/variable_inspector.cpp
        debugger/debug_adapter_protocol.cpp
        # Terminal
        terminal/pty_handler.cpp
        terminal/shell_integration.cpp
        terminal/terminal_emulator.cpp
        # Networking
        networking/http_client.cpp
        networking/websocket_client.cpp
        networking/live_share.cpp
        networking/remote_dev.cpp
    )

    local cxxflags=(
        -std=c++20
        -Wall -Wextra
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        "-I$ROOT_DIR"
        "-I$ROOT_DIR/core"
        "-I$ROOT_DIR/ui"
        "-I$ROOT_DIR/editor"
        "-I$ROOT_DIR/ai"
        "-I$ROOT_DIR/extensions"
    )

    case "$BUILD_TYPE" in
        Release)        cxxflags+=(-O2 -DNDEBUG) ;;
        Debug)          cxxflags+=(-g -O0) ;;
        RelWithDebInfo) cxxflags+=(-O2 -g -DNDEBUG) ;;
    esac

    local ldflags=()

    case "$PLATFORM" in
        Linux)
            cxxflags+=(-DNEXUS_PLATFORM_LINUX)
            sources+=(core/platform/platform_linux.cpp)
            ldflags+=(-lpthread -ldl)
            ;;
        macOS)
            cxxflags+=(-DNEXUS_PLATFORM_MACOS)
            sources+=(core/platform/platform_macos.cpp)
            ldflags+=(-lpthread -framework Cocoa -framework Metal -framework MetalKit -framework QuartzCore -framework IOKit -framework CoreFoundation)
            ;;
        Windows)
            cxxflags+=(-DNEXUS_PLATFORM_WINDOWS)
            sources+=(core/platform/platform_windows.cpp)
            ldflags+=(-lpthread -ldwmapi -ld3d11 -ldxgi -ld3dcompiler -lshlwapi)
            ;;
        *)
            ldflags+=(-lpthread)
            ;;
    esac

    if [[ "$ENABLE_AI" -eq 1 ]]; then
        cxxflags+=(-DNEXUS_ENABLE_AI)
        sources+=(
            ai/features/ai_autocomplete.cpp
            ai/features/ai_chat.cpp
            ai/providers/openai_provider.cpp
            ai/providers/anthropic_provider.cpp
            ai/providers/local_llm_provider.cpp
        )
    fi

    if [[ "$ENABLE_VULKAN" -eq 1 ]]; then
        if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists vulkan; then
            # shellcheck disable=SC2207
            cxxflags+=(-DNEXUS_ENABLE_VULKAN -DNEXUS_HAS_VULKAN $(pkg-config --cflags vulkan))
            # shellcheck disable=SC2207
            ldflags+=($(pkg-config --libs vulkan))
            sources+=(ui/renderer/vulkan_backend.cpp)
        elif printf '#include <vulkan/vulkan.h>\n' | "$CXX_BIN" -x c++ -fsyntax-only - >/dev/null 2>&1; then
            cxxflags+=(-DNEXUS_ENABLE_VULKAN -DNEXUS_HAS_VULKAN)
            ldflags+=(-lvulkan)
            sources+=(ui/renderer/vulkan_backend.cpp)
        else
            echo "Error: --vulkan requested, but <vulkan/vulkan.h> was not found." >&2
            echo "Install the Vulkan development headers (e.g. libvulkan-dev) or omit --vulkan." >&2
            exit 1
        fi
    fi

    if [[ "$ENABLE_OPENGL" -eq 1 ]]; then
        if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists gl; then
            # shellcheck disable=SC2207
            cxxflags+=(-DNEXUS_ENABLE_OPENGL -DNEXUS_HAS_OPENGL $(pkg-config --cflags gl))
            # shellcheck disable=SC2207
            ldflags+=($(pkg-config --libs gl))
            sources+=(ui/renderer/opengl_backend.cpp)
        elif printf '#include <GL/gl.h>\n' | "$CXX_BIN" -x c++ -fsyntax-only - >/dev/null 2>&1; then
            cxxflags+=(-DNEXUS_ENABLE_OPENGL -DNEXUS_HAS_OPENGL)
            ldflags+=(-lGL)
            sources+=(ui/renderer/opengl_backend.cpp)
        else
            echo "Error: --opengl requested, but <GL/gl.h> was not found." >&2
            echo "Install the OpenGL development headers (e.g. libgl1-mesa-dev) or omit --opengl." >&2
            exit 1
        fi
    fi

    if [[ "$ENABLE_METAL" -eq 1 && "$PLATFORM" == "macOS" ]]; then
        cxxflags+=(-DNEXUS_ENABLE_METAL)
    fi

    if [[ "$ENABLE_DX12" -eq 1 && "$PLATFORM" == "Windows" ]]; then
        cxxflags+=(-DNEXUS_ENABLE_DX12)
        sources+=(ui/renderer/directx_backend.cpp)
    fi

    if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists freetype2; then
        # shellcheck disable=SC2207
        cxxflags+=(-DNEXUS_HAS_FREETYPE $(pkg-config --cflags freetype2))
        # shellcheck disable=SC2207
        ldflags+=($(pkg-config --libs freetype2))
    fi

    if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists tree-sitter; then
        # shellcheck disable=SC2207
        cxxflags+=(-DNEXUS_HAS_TREE_SITTER $(pkg-config --cflags tree-sitter))
        # shellcheck disable=SC2207
        ldflags+=($(pkg-config --libs tree-sitter))
    fi

    # Detect compiler/flag changes so incremental builds stay accurate
    local flags_sig="$CXX_BIN ${cxxflags[*]}"
    local flags_file="$BUILD_DIR/.build_flags"
    local force_rebuild=0
    if [[ ! -f "$flags_file" ]] || [[ "$(cat "$flags_file")" != "$flags_sig" ]]; then
        force_rebuild=1
        printf '%s' "$flags_sig" > "$flags_file"
    fi

    # Determine which objects need (re)compilation
    local to_compile=()
    local all_objs=()
    for src in "${sources[@]}"; do
        local obj="$obj_dir/${src//\//_}.o"
        local dep="$obj_dir/${src//\//_}.d"
        all_objs+=("$obj")

        local needs_build=0
        if [[ "$force_rebuild" -eq 1 || ! -f "$obj" || "$ROOT_DIR/$src" -nt "$obj" ]]; then
            needs_build=1
        elif [[ -f "$dep" ]]; then
            # Check header dependencies recorded in .d file
            local dep_file
            for dep_file in $(sed -e 's/^[^:]*://' -e 's/\\$//' "$dep"); do
                if [[ -f "$dep_file" && "$dep_file" -nt "$obj" ]]; then
                    needs_build=1
                    break
                fi
            done
        fi

        if [[ "$needs_build" -eq 1 ]]; then
            to_compile+=("$src")
        fi
    done

    local total="${#sources[@]}"
    local dirty="${#to_compile[@]}"

    if [[ "$dirty" -gt 0 ]]; then
        echo "[direct] Compiling $dirty/$total source files with $JOBS parallel jobs..."
        # Export settings for parallel worker subshells
        export NF_ROOT="$ROOT_DIR"
        export NF_OBJ_DIR="$obj_dir"
        export NF_CXX="$CXX_BIN"
        export NF_VERBOSE="$VERBOSE"
        export NF_CXXFLAGS="${cxxflags[*]}"

        printf '%s\n' "${to_compile[@]}" | xargs -P "$JOBS" -I {} bash -c '
            set -euo pipefail
            src="$1"
            obj="$NF_OBJ_DIR/${src//\//_}.o"
            dep="$NF_OBJ_DIR/${src//\//_}.d"
            # shellcheck disable=SC2206
            flags=($NF_CXXFLAGS)
            if [[ "$NF_VERBOSE" -eq 1 ]]; then
                echo "$NF_CXX ${flags[*]} -MMD -MP -MF $dep -c $NF_ROOT/$src -o $obj"
            else
                echo "  [CXX] $src"
            fi
            "$NF_CXX" "${flags[@]}" -MMD -MP -MF "$dep" -c "$NF_ROOT/$src" -o "$obj"
        ' _ {}
    else
        echo "[direct] All $total object files are up to date."
    fi

    # Link if binary is missing or any object is newer
    local needs_link=0
    if [[ ! -f "$BIN_PATH" || "$dirty" -gt 0 ]]; then
        needs_link=1
    else
        for obj in "${all_objs[@]}"; do
            if [[ "$obj" -nt "$BIN_PATH" ]]; then
                needs_link=1
                break
            fi
        done
    fi

    if [[ "$needs_link" -eq 1 ]]; then
        echo "[direct] Linking $BIN_PATH..."
        if [[ "$VERBOSE" -eq 1 ]]; then
            echo "$CXX_BIN -std=c++20 ${all_objs[*]} ${ldflags[*]} -o $BIN_PATH"
        fi
        "$CXX_BIN" -std=c++20 "${all_objs[@]}" "${ldflags[@]}" -o "$BIN_PATH"
    else
        echo "[direct] Binary is up to date: $BIN_PATH"
    fi
}

run_checks() {
    resolve_compiler
    echo "[check] Verifying all headers compile cleanly together (ODR check)..."
    local check_tu="$BUILD_DIR/all_headers_check.cpp"
    {
        while IFS= read -r h; do
            echo "#include \"${h#"$ROOT_DIR/"}\""
        done < <(find "$ROOT_DIR" -not -path "$ROOT_DIR/build/*" -not -path "$ROOT_DIR/out/*" -not -path "$ROOT_DIR/.git/*" -name '*.hpp' | sort)
        echo "int nexus_headers_check_ok() { return 0; }"
    } > "$check_tu"

    "$CXX_BIN" -std=c++20 -Wall -Wextra -Wno-unused-parameter "-I$ROOT_DIR" -fsyntax-only "$check_tu"
    echo "[check] All headers passed combined syntax & ODR check."

    if [[ -x "$BIN_PATH" ]]; then
        echo "[check] Running smoke test: $BIN_PATH --version"
        "$BIN_PATH" --version
    fi
}

START_TS="$(date +%s)"

if [[ "$BACKEND" == "cmake" ]]; then
    build_with_cmake
else
    build_direct
fi

END_TS="$(date +%s)"
echo "[done] Built $BIN_PATH in $((END_TS - START_TS))s"

if [[ "$RUN_CHECK" -eq 1 ]]; then
    run_checks
fi

if [[ "$RUN_AFTER_BUILD" -eq 1 ]]; then
    if [[ "${#RUN_ARGS[@]}" -gt 0 ]]; then
        echo "[run] Executing $BIN_PATH ${RUN_ARGS[*]}"
        exec "$BIN_PATH" "${RUN_ARGS[@]}"
    else
        echo "[run] Executing $BIN_PATH"
        exec "$BIN_PATH"
    fi
fi
