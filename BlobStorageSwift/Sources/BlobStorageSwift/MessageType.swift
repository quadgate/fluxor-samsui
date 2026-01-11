/// MessageType - Enumeration of message types
public enum MessageType: UInt8, CaseIterable {
    case shortMessage = 0
    case longMessage = 1
    case image = 2
    case video = 3
}
