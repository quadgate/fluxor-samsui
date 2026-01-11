# BlobStorageSwift

A Swift Package for binary message storage, compatible with the Android/Kotlin implementation.

## Features

- Binary file storage for efficient message persistence
- Compatible binary format with Android implementation
- Thread-safe operations
- iOS and macOS support
- Zero external dependencies

## Installation

### Swift Package Manager

Add the following to your `Package.swift`:

```swift
dependencies: [
    .package(url: "path/to/BlobStorageSwift", from: "1.0.0")
]
```

Or add it via Xcode:
1. File → Add Packages...
2. Enter the package URL
3. Select version and add to target

## Usage

### Basic Usage

```swift
import BlobStorageSwift

// Initialize with default documents directory
let storage = BlobStorage()

// Or initialize with custom directory
let customDir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
let storage = BlobStorage(baseDirectory: customDir)

// Save messages
let messages = [
    Message(text: "Hello", isSent: true, messageType: .shortMessage),
    Message(text: "World", isSent: false, messageType: .longMessage)
]
try storage.saveMessages(messages)

// Load messages
let loadedMessages = try storage.loadMessages()

// Clear messages
try storage.clearMessages()

// Check if messages exist
if try storage.hasMessages() {
    print("Messages exist in storage")
}

// Get storage size
let size = try storage.getStorageSize()
print("Storage size: \(size) bytes")
```

### Message Types

```swift
enum MessageType: UInt8 {
    case shortMessage  // Messages < 1024 characters
    case longMessage   // Messages >= 1024 characters
    case image         // Image messages
    case video         // Video messages
}
```

### Message Structure

```swift
struct Message {
    let text: String
    let isSent: Bool
    let messageType: MessageType
    let timestamp: Int64
}
```

## Binary Format

The storage uses a binary format compatible with the Android implementation:

```
[message_count: Int32 (4 bytes)]
For each message:
  [text_length: Int32 (4 bytes)]
  [text: UTF-8 bytes (text_length bytes)]
  [is_sent: UInt8 (1 byte)]  // 0 = false, 1 = true
  [message_type: UInt8 (1 byte)]  // enum ordinal
  [timestamp: Int64 (8 bytes)]
```

## Requirements

- iOS 13.0+
- macOS 10.15+
- Swift 5.9+

## License

See LICENSE file for details.

## Compatibility

This Swift implementation uses the same binary format as the Android/Kotlin version, allowing data to be shared between platforms (with appropriate file transfer mechanisms).
