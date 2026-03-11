# NexusForge IDE

<div align="center">

![NexusForge IDE](https://img.shields.io/badge/NexusForge-IDE-blue)
![C++20](https://img.shields.io/badge/C%2B%2B-20-green)
![Platforms](https://img.shields.io/badge/Platforms-Windows%20%7C%20macOS%20%7C%20Linux%20%7C%20Android%20%7C%20iOS-lightgrey)
![License](https://img.shields.io/badge/License-MIT-yellow)

**A Revolutionary Cross-Platform IDE with Mobile-First Design & AI-Powered Features**

[Features](#features) • [Building](#building) • [Documentation](#documentation) • [Contributing](#contributing)

</div>

---

## 📖 Overview

**NexusForge IDE** is an enterprise-grade, AI-powered integrated development environment built from the ground up with a mobile-first approach. Written primarily in modern C++20, it delivers native performance across all major platforms while providing an intelligent, adaptive user interface.

### Key Highlights

- 🚀 **High Performance** - Custom memory manager, thread pool, and SIMD optimizations
- 📱 **Mobile-First** - Adaptive UI that works seamlessly on phones, tablets, and desktops
- 🤖 **AI-Powered** - Built-in AI autocomplete, chat, refactoring, and code analysis
- 🔌 **Extensible** - VS Code-compatible extension API with sandboxed execution
- 🎨 **Modern UI** - Custom rendering engine with Vulkan, OpenGL, Metal, and DirectX backends
- 🌍 **Cross-Platform** - Windows, macOS, Linux, Android, iOS, and Web support

---

## ✨ Features

### Core Editor
- Advanced text buffer with gap buffer, rope, and piece table implementations
- Syntax highlighting powered by Tree-sitter
- Language Server Protocol (LSP) integration
- Multi-cursor editing and column selection
- Code folding with multiple providers
- Bracket matching and auto-closing
- Find/replace with regex support
- Mini-map for code navigation
- Full undo/redo with group support

### AI Integration
- **AI Autocomplete** - Context-aware code suggestions
- **AI Chat** - Interactive coding assistant
- **Code Explanation** - Understand complex code instantly
- **Smart Refactoring** - AI-powered code improvements
- **Bug Detection** - Automatic issue identification
- **Test Generation** - Auto-generate unit tests
- **Documentation** - AI-generated code documentation
- Multiple providers: OpenAI, Anthropic, and local LLMs

### UI Framework
- Custom widget system with theming support
- Flexbox, Grid, and Box layout engines
- Smooth animations with easing functions
- Touch gestures: tap, swipe, pinch, rotate
- Adaptive layouts for all screen sizes
- Dark/Light/High-Contrast themes

### Extension System
- VS Code-compatible extension API
- Sandboxed JavaScript/WebAssembly execution
- Extension marketplace
- IPC communication channel
- Full access to editor APIs

### Platform Support
| Platform | Status | Backend |
|----------|--------|---------|
| Windows | ✅ | DirectX 12, Vulkan, OpenGL |
| macOS | ✅ | Metal, OpenGL |
| Linux | ✅ | Vulkan, OpenGL |
| Android | ✅ | OpenGL ES |
| iOS | ✅ | Metal |
| Web | 🚧 | WebGL |

---

## 🏗️ Building

### Prerequisites

**All Platforms:**
- CMake 3.20+
- C++20 compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)

**Windows:**
- Visual Studio 2022 or later
- Windows SDK 10+
- Vulkan SDK (optional)

**Linux:**
```bash
# Ubuntu/Debian
sudo apt install build-essential cmake libx11-dev libxrandr-dev libxi-dev \
                 libxcursor-dev libxinerama-dev libgl1-mesa-dev \
                 libfreetype6-dev libvulkan-dev

# Fedora
sudo dnf install gcc-c++ cmake libX11-devel libXrandr-devel libXi-devel \
                 libXcursor-devel libXinerama-devel mesa-libGL-devel \
                 freetype-devel vulkan-devel
```

**macOS:**
```bash
brew install cmake
# Xcode Command Line Tools required
xcode-select --install
```

**Android:**
- Android NDK r25+
- Android SDK

**iOS:**
- Xcode 15+
- iOS SDK

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/devTechs001/nexus-forge-ide.git
cd nexus-forge-ide/NexusForge

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Optional features
# -DNEXUS_ENABLE_VULKAN=ON      # Enable Vulkan renderer
# -DNEXUS_ENABLE_OPENGL=ON      # Enable OpenGL renderer
# -DNEXUS_ENABLE_METAL=ON       # Enable Metal renderer (macOS/iOS)
# -DNEXUS_ENABLE_DX12=ON        # Enable DirectX 12 (Windows)
# -DNEXUS_ENABLE_AI=ON          # Enable AI features
# -DNEXUS_ENABLE_ONNX=ON        # Enable ONNX Runtime
# -DNEXUS_ENABLE_LLAMA=ON       # Enable local LLM support
# -DNEXUS_BUILD_TESTS=ON        # Build unit tests

# Build
cmake --build . --config Release

# Install
cmake --install .
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `NEXUS_BUILD_TESTS` | ON | Build unit tests |
| `NEXUS_BUILD_DOCS` | OFF | Build documentation |
| `NEXUS_ENABLE_VULKAN` | ON | Enable Vulkan renderer |
| `NEXUS_ENABLE_OPENGL` | ON | Enable OpenGL renderer |
| `NEXUS_ENABLE_METAL` | ON | Enable Metal renderer |
| `NEXUS_ENABLE_DX12` | ON | Enable DirectX 12 |
| `NEXUS_ENABLE_AI` | ON | Enable AI features |
| `NEXUS_ENABLE_ONNX` | ON | Enable ONNX Runtime |
| `NEXUS_ENABLE_LLAMA` | ON | Enable local LLM |
| `NEXUS_ENABLE_TELEMETRY` | OFF | Enable telemetry |

---

## 🎯 Architecture

### Core Engine
The heart of NexusForge is a custom engine featuring:
- **Memory Manager** - Pool allocators, arena allocators, leak detection
- **Thread Pool** - Efficient parallel task execution
- **Event System** - Type-safe, priority-based event handling
- **Plugin Loader** - Dynamic plugin loading with dependency management

### Rendering
Multi-backend rendering architecture:
- **Vulkan** - High-performance, cross-platform
- **OpenGL** - Wide compatibility
- **Metal** - Apple platforms optimization
- **DirectX 12** - Windows optimization

### Text Processing
Multiple buffer implementations for different use cases:
- **Gap Buffer** - Efficient for cursor-based editing
- **Rope** - Optimal for large files
- **Piece Table** - Balanced performance

---

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

### Quick Start
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Code Style
- Follow the existing C++ code style
- Use meaningful variable and function names
- Add comments for complex logic
- Write tests for new features

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- [Tree-sitter](https://tree-sitter.github.io/) - Incremental parsing
- [GLFW](https://www.glfw.org/) - Window management
- [FreeType](https://freetype.org/) - Font rendering
- [nlohmann/json](https://github.com/nlohmann/json) - JSON handling

---

## 📬 Contact

- **Website**: [nexusforge.dev](https://nexusforge.dev)
- **Twitter**: [@NexusForgeIDE](https://twitter.com/NexusForgeIDE)
- **Discord**: [Join our server](https://discord.gg/nexusforge)
- **Email**: team@nexusforge.dev

---

<div align="center">

**Built with ❤️ by the NexusForge Team**

⭐ Star this repo if you find it useful!

</div>
