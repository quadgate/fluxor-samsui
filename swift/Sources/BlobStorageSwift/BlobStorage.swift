import Foundation

/// BlobStorage - Handles local persistence of messages using binary file storage
public class BlobStorage {
    
    private static let storageDir = "messages"
    private static let messagesFile = "messages.blob"
    
    private let baseDirectory: URL
    private let messagesFileURL: URL
    
    /// Initialize BlobStorage with a base directory (e.g., app's documents directory)
    /// - Parameter baseDirectory: Base directory URL where messages directory will be created
    public init(baseDirectory: URL) {
        self.baseDirectory = baseDirectory
        let messagesDir = baseDirectory.appendingPathComponent(BlobStorage.storageDir, isDirectory: true)
        self.messagesFileURL = messagesDir.appendingPathComponent(BlobStorage.messagesFile, isDirectory: false)
    }
    
    /// Initialize BlobStorage using FileManager's default documents directory
    public convenience init() {
        let documentsDirectory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        self.init(baseDirectory: documentsDirectory)
    }
    
    /// Ensure the messages directory exists
    private func ensureDirectoryExists() throws {
        let messagesDir = messagesFileURL.deletingLastPathComponent()
        try FileManager.default.createDirectory(at: messagesDir, withIntermediateDirectories: true, attributes: nil)
    }
    
    /// Serialize messages to byte array using the same binary format as Kotlin/Android
    private func serializeMessages(_ messages: [Message]) -> Data {
        var data = Data()
        
        // Write message count (Int32, 4 bytes) - little-endian (matches Android)
        var count = Int32(messages.count).littleEndian
        data.append(Data(bytes: &count, count: MemoryLayout<Int32>.size))
        
        // Write each message
        for message in messages {
            // Write text length and text bytes (UTF-8) - little-endian
            let textData = message.text.data(using: .utf8) ?? Data()
            var textLength = Int32(textData.count).littleEndian
            data.append(Data(bytes: &textLength, count: MemoryLayout<Int32>.size))
            data.append(textData)
            
            // Write isSent (1 byte: 0 = false, 1 = true)
            let isSentByte: UInt8 = message.isSent ? 1 : 0
            data.append(isSentByte)
            
            // Write messageType (1 byte: enum ordinal)
            let messageTypeByte = message.messageType.rawValue
            data.append(messageTypeByte)
            
            // Write timestamp (Int64, 8 bytes) - little-endian
            var timestamp = message.timestamp.littleEndian
            data.append(Data(bytes: &timestamp, count: MemoryLayout<Int64>.size))
        }
        
        return data
    }
    
    /// Deserialize messages from byte array using the same binary format as Kotlin/Android
    private func deserializeMessages(_ data: Data) throws -> [Message] {
        guard !data.isEmpty else {
            return []
        }
        
        var offset = 0
        var messages: [Message] = []
        
        // Read message count (Int32, 4 bytes) - ensure little-endian (matches Android)
        guard offset + MemoryLayout<Int32>.size <= data.count else {
            throw BlobStorageError.invalidFormat("Cannot read message count")
        }
        let countData = data.subdata(in: offset..<offset + MemoryLayout<Int32>.size)
        let count = countData.withUnsafeBytes { bytes in
            Int32(littleEndian: bytes.load(as: Int32.self))
        }
        offset += MemoryLayout<Int32>.size
        
        // Read each message
        for _ in 0..<Int(count) {
            // Read text length (Int32, 4 bytes) - ensure little-endian
            guard offset + MemoryLayout<Int32>.size <= data.count else {
                throw BlobStorageError.invalidFormat("Cannot read text length")
            }
            let textLengthData = data.subdata(in: offset..<offset + MemoryLayout<Int32>.size)
            let textLength = textLengthData.withUnsafeBytes { bytes in
                Int32(littleEndian: bytes.load(as: Int32.self))
            }
            offset += MemoryLayout<Int32>.size
            
            // Read text bytes (UTF-8)
            guard offset + Int(textLength) <= data.count else {
                throw BlobStorageError.invalidFormat("Cannot read text data")
            }
            let textData = data.subdata(in: offset..<offset + Int(textLength))
            guard let text = String(data: textData, encoding: .utf8) else {
                throw BlobStorageError.invalidFormat("Invalid UTF-8 text data")
            }
            offset += Int(textLength)
            
            // Read isSent (1 byte)
            guard offset < data.count else {
                throw BlobStorageError.invalidFormat("Cannot read isSent")
            }
            let isSentByte = data[offset]
            let isSent = isSentByte == 1
            offset += 1
            
            // Read messageType (1 byte)
            guard offset < data.count else {
                throw BlobStorageError.invalidFormat("Cannot read messageType")
            }
            let messageTypeByte = data[offset]
            let messageType = MessageType(rawValue: messageTypeByte) ?? .shortMessage
            offset += 1
            
            // Read timestamp (Int64, 8 bytes) - ensure little-endian
            guard offset + MemoryLayout<Int64>.size <= data.count else {
                throw BlobStorageError.invalidFormat("Cannot read timestamp")
            }
            let timestampData = data.subdata(in: offset..<offset + MemoryLayout<Int64>.size)
            let timestamp = timestampData.withUnsafeBytes { bytes in
                Int64(littleEndian: bytes.load(as: Int64.self))
            }
            offset += MemoryLayout<Int64>.size
            
            messages.append(Message(
                text: text,
                isSent: isSent,
                messageType: messageType,
                timestamp: timestamp
            ))
        }
        
        return messages
    }
    
    /// Save messages to blob storage
    /// - Parameter messages: List of messages to save
    /// - Throws: BlobStorageError if save fails
    public func saveMessages(_ messages: [Message]) throws {
        try ensureDirectoryExists()
        
        let data = serializeMessages(messages)
        try data.write(to: messagesFileURL, options: [.atomic])
    }
    
    /// Load messages from blob storage
    /// - Returns: List of messages loaded from storage
    /// - Throws: BlobStorageError if load fails
    public func loadMessages() throws -> [Message] {
        guard FileManager.default.fileExists(atPath: messagesFileURL.path) else {
            return []
        }
        
        let data = try Data(contentsOf: messagesFileURL)
        
        guard !data.isEmpty else {
            return []
        }
        
        do {
            return try deserializeMessages(data)
        } catch {
            // If deserialization fails, clear corrupted file
            try? clearMessages()
            throw error
        }
    }
    
    /// Clear all stored messages
    /// - Throws: BlobStorageError if clear fails
    public func clearMessages() throws {
        guard FileManager.default.fileExists(atPath: messagesFileURL.path) else {
            return // File doesn't exist, nothing to clear
        }
        
        try FileManager.default.removeItem(at: messagesFileURL)
    }
    
    /// Check if messages exist in storage
    /// - Returns: true if file exists and has size > 0, false otherwise
    /// - Throws: BlobStorageError if file system error occurs
    public func hasMessages() throws -> Bool {
        guard FileManager.default.fileExists(atPath: messagesFileURL.path) else {
            return false
        }
        
        do {
            let attributes = try FileManager.default.attributesOfItem(atPath: messagesFileURL.path)
            guard let fileSize = attributes[.size] as? Int64 else {
                return false
            }
            return fileSize > 0
        } catch {
            throw BlobStorageError.fileSystemError(error)
        }
    }
    
    /// Get storage file size in bytes
    /// - Returns: File size in bytes, or 0 if file doesn't exist
    /// - Throws: BlobStorageError if file system error occurs
    public func getStorageSize() throws -> Int64 {
        guard FileManager.default.fileExists(atPath: messagesFileURL.path) else {
            return 0
        }
        
        do {
            let attributes = try FileManager.default.attributesOfItem(atPath: messagesFileURL.path)
            guard let fileSize = attributes[.size] as? Int64 else {
                return 0
            }
            return fileSize
        } catch {
            throw BlobStorageError.fileSystemError(error)
        }
    }
}

/// BlobStorageError - Errors that can occur during blob storage operations
public enum BlobStorageError: LocalizedError {
    case invalidFormat(String)
    case fileSystemError(Error)
    
    public var errorDescription: String? {
        switch self {
        case .invalidFormat(let message):
            return "Invalid blob storage format: \(message)"
        case .fileSystemError(let error):
            return "File system error: \(error.localizedDescription)"
        }
    }
}
