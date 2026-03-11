// Package.swift - iOS/macOS Package Configuration for NexusForge IDE
// swift-tools-version:5.9

import PackageDescription

let package = Package(
    name: "NexusForge",
    platforms: [
        .macOS(.v13),
        .iOS(.v16),
        .macCatalyst(.v16)
    ],
    products: [
        .library(
            name: "NexusForgeCore",
            targets: ["NexusForgeCore"]
        ),
        .executable(
            name: "NexusForge",
            targets: ["NexusForgeApp"]
        )
    ],
    dependencies: [
        // Swift NIO for async networking
        .package(url: "https://github.com/apple/swift-nio.git", from: "2.60.0"),
        
        // Swift Log
        .package(url: "https://github.com/apple/swift-log.git", from: "1.5.0"),
        
        // Swift Metrics
        .package(url: "https://github.com/apple/swift-metrics.git", from: "2.4.0"),
        
        // Swift Collections
        .package(url: "https://github.com/apple/swift-collections.git", from: "1.0.0"),
        
        // Swift Algorithms
        .package(url: "https://github.com/apple/swift-algorithms.git", from: "1.0.0"),
        
        // Async HTTP client
        .package(url: "https://github.com/swift-server/async-http-client.git", from: "1.18.0"),
        
        // WebSocket
        .package(url: "https://github.com/vapor/websocket-kit.git", from: "2.13.0"),
        
        // JSON
        .package(url: "https://github.com/apple/swift-json.git", from: "1.0.0"),
        
        // Crypto
        .package(url: "https://github.com/apple/swift-crypto.git", from: "3.0.0"),
        
        // LSP (Language Server Protocol)
        .package(url: "https://github.com/apple/swift-lsp.git", from: "0.1.0"),
        
        // Tree-sitter Swift bindings
        .package(url: "https://github.com/ChimeHQ/SwiftTreeSitter.git", from: "0.8.0"),
    ],
    targets: [
        // Core library target
        .target(
            name: "NexusForgeCore",
            dependencies: [
                .product(name: "NIO", package: "swift-nio"),
                .product(name: "Logging", package: "swift-log"),
                .product(name: "Metrics", package: "swift-metrics"),
                .product(name: "Collections", package: "swift-collections"),
                .product(name: "Algorithms", package: "swift-algorithms"),
                .product(name: "AsyncHTTPClient", package: "async-http-client"),
                .product(name: "WebSocketKit", package: "websocket-kit"),
                .product(name: "Crypto", package: "swift-crypto"),
                .product(name: "SwiftTreeSitter", package: "SwiftTreeSitter"),
            ],
            path: "Sources/Core",
            swiftSettings: [
                .define("NEXUS_PLATFORM_MACOS"),
                .unsafeFlags(["-enable-experimental-concurrency"])
            ]
        ),
        
        // Text editing engine
        .target(
            name: "NexusForgeEditor",
            dependencies: ["NexusForgeCore"],
            path: "Sources/Editor",
            swiftSettings: [
                .define("NEXUS_PLATFORM_MACOS")
            ]
        ),
        
        // UI Framework (SwiftUI based)
        .target(
            name: "NexusForgeUI",
            dependencies: ["NexusForgeCore"],
            path: "Sources/UI",
            swiftSettings: [
                .define("NEXUS_PLATFORM_MACOS")
            ]
        ),
        
        // AI Integration
        .target(
            name: "NexusForgeAI",
            dependencies: ["NexusForgeCore"],
            path: "Sources/AI",
            swiftSettings: [
                .define("NEXUS_PLATFORM_MACOS")
            ]
        ),
        
        // Main app target
        .executableTarget(
            name: "NexusForgeApp",
            dependencies: [
                "NexusForgeCore",
                "NexusForgeEditor",
                "NexusForgeUI",
                "NexusForgeAI",
            ],
            path: "Sources/App",
            swiftSettings: [
                .define("NEXUS_PLATFORM_MACOS")
            ],
            linkerSettings: [
                .linkedFramework("AppKit", .when(platforms: [.macOS])),
                .linkedFramework("UIKit", .when(platforms: [.iOS])),
                .linkedFramework("Metal"),
                .linkedFramework("MetalKit"),
            ]
        ),
        
        // Tests
        .testTarget(
            name: "NexusForgeTests",
            dependencies: ["NexusForgeCore"],
            path: "Tests"
        ),
    ],
    swiftLanguageVersions: [.v5]
)
