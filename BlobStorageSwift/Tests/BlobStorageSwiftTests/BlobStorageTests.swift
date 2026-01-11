import XCTest
@testable import BlobStorageSwift

final class BlobStorageTests: XCTestCase {
    
    var storage: BlobStorage!
    var tempDirectory: URL!
    
    override func setUp() {
        super.setUp()
        // Create a temporary directory for testing
        tempDirectory = FileManager.default.temporaryDirectory
            .appendingPathComponent(UUID().uuidString, isDirectory: true)
        try? FileManager.default.createDirectory(at: tempDirectory, withIntermediateDirectories: true)
        storage = BlobStorage(baseDirectory: tempDirectory)
    }
    
    override func tearDown() {
        // Clean up temporary directory
        try? FileManager.default.removeItem(at: tempDirectory)
        storage = nil
        tempDirectory = nil
        super.tearDown()
    }
    
    func testSaveAndLoadMessages() throws {
        let messages = [
            Message(text: "Hello", isSent: true, messageType: .shortMessage),
            Message(text: "World", isSent: false, messageType: .longMessage),
            Message(text: "Test image", isSent: true, messageType: .image)
        ]
        
        try storage.saveMessages(messages)
        
        let loadedMessages = try storage.loadMessages()
        
        XCTAssertEqual(loadedMessages.count, messages.count)
        XCTAssertEqual(loadedMessages[0].text, "Hello")
        XCTAssertEqual(loadedMessages[0].isSent, true)
        XCTAssertEqual(loadedMessages[0].messageType, .shortMessage)
        XCTAssertEqual(loadedMessages[1].text, "World")
        XCTAssertEqual(loadedMessages[1].isSent, false)
        XCTAssertEqual(loadedMessages[1].messageType, .longMessage)
    }
    
    func testLoadEmptyStorage() throws {
        let messages = try storage.loadMessages()
        XCTAssertTrue(messages.isEmpty)
    }
    
    func testClearMessages() throws {
        let messages = [
            Message(text: "Test", isSent: true)
        ]
        
        try storage.saveMessages(messages)
        XCTAssertTrue(try storage.hasMessages())
        
        try storage.clearMessages()
        XCTAssertFalse(try storage.hasMessages())
        
        let loadedMessages = try storage.loadMessages()
        XCTAssertTrue(loadedMessages.isEmpty)
    }
    
    func testHasMessages() throws {
        XCTAssertFalse(try storage.hasMessages())
        
        let messages = [Message(text: "Test", isSent: true)]
        try storage.saveMessages(messages)
        
        XCTAssertTrue(try storage.hasMessages())
    }
    
    func testGetStorageSize() throws {
        XCTAssertEqual(try storage.getStorageSize(), 0)
        
        let messages = [Message(text: "Test", isSent: true)]
        try storage.saveMessages(messages)
        
        let size = try storage.getStorageSize()
        XCTAssertGreaterThan(size, 0)
    }
    
    func testMessageEquality() {
        let message1 = Message(text: "Hello", isSent: true, messageType: .shortMessage, timestamp: 12345)
        let message2 = Message(text: "Hello", isSent: true, messageType: .shortMessage, timestamp: 12345)
        let message3 = Message(text: "World", isSent: true, messageType: .shortMessage, timestamp: 12345)
        
        XCTAssertEqual(message1, message2)
        XCTAssertNotEqual(message1, message3)
    }
    
    func testMessageTypeEnum() {
        XCTAssertEqual(MessageType.shortMessage.rawValue, 0)
        XCTAssertEqual(MessageType.longMessage.rawValue, 1)
        XCTAssertEqual(MessageType.image.rawValue, 2)
        XCTAssertEqual(MessageType.video.rawValue, 3)
        
        XCTAssertEqual(MessageType(rawValue: 0), .shortMessage)
        XCTAssertEqual(MessageType(rawValue: 1), .longMessage)
        XCTAssertEqual(MessageType(rawValue: 2), .image)
        XCTAssertEqual(MessageType(rawValue: 3), .video)
    }
}
