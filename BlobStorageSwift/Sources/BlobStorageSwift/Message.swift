import Foundation

/// Message - Data structure representing a single message
public struct Message: Codable, Equatable {
    public let text: String
    public let isSent: Bool
    public let messageType: MessageType
    public let timestamp: Int64
    
    public init(
        text: String,
        isSent: Bool,
        messageType: MessageType = .shortMessage,
        timestamp: Int64 = Int64(Date().timeIntervalSince1970 * 1000)
    ) {
        self.text = text
        self.isSent = isSent
        self.messageType = messageType
        self.timestamp = timestamp
    }
}
