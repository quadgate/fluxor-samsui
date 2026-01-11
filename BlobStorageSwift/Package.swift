// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "BlobStorageSwift",
    platforms: [
        .iOS(.v13),
        .macOS(.v10_15)
    ],
    products: [
        .library(
            name: "BlobStorageSwift",
            targets: ["BlobStorageSwift"]),
    ],
    dependencies: [
        // No external dependencies
    ],
    targets: [
        .target(
            name: "BlobStorageSwift",
            dependencies: []),
        .testTarget(
            name: "BlobStorageSwiftTests",
            dependencies: ["BlobStorageSwift"]),
    ]
)
