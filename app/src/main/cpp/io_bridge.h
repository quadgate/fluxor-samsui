#ifndef IO_BRIDGE_H
#define IO_BRIDGE_H

#include <jni.h>
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>
#include <atomic>
#include <cstdint>
#include "message_encryption.h"

// Forward declaration
class ThreadManager;

enum class EventType {
    STRING,
    INT,
    FLOAT,
    DOUBLE,
    BOOLEAN,
    BYTE_ARRAY
};

struct Event {
    EventType type;
    std::string eventId;
    
    // Union for different event data types
    union {
        int32_t intValue;
        float floatValue;
        double doubleValue;
        bool boolValue;
    };
    std::string stringValue;
    std::vector<uint8_t> byteArrayValue;
    
    Event() : type(EventType::STRING), intValue(0) {}
};

class IOBridge {
public:
    IOBridge();
    ~IOBridge();
    
    // Disable copy constructor and assignment operator
    IOBridge(const IOBridge&) = delete;
    IOBridge& operator=(const IOBridge&) = delete;
    
    // Initialization
    void initialize(JavaVM* jvm);
    void cleanup();
    
    // Listener registration
    void registerListener(JNIEnv* env, jobject listener);
    void unregisterListener(JNIEnv* env);
    
    // Event posting methods (called from C++ threads)
    void postStringEvent(const std::string& eventId, const std::string& data);
    void postIntEvent(const std::string& eventId, int32_t data);
    void postFloatEvent(const std::string& eventId, float data);
    void postDoubleEvent(const std::string& eventId, double data);
    void postBooleanEvent(const std::string& eventId, bool data);
    void postByteArrayEvent(const std::string& eventId, const uint8_t* data, size_t length);
    
    // Process events (internal, called by ThreadManager)
    void processEvents();
    
    // Set thread manager reference
    void setThreadManager(ThreadManager* threadManager);
    
    // Encryption control
    void enableEncryption(bool enable);
    
    // Check if initialized
    bool isInitialized() const;

private:
    // JVM and listener references
    JavaVM* jvm_;
    jobject listenerObject_;
    jclass listenerClass_;
    
    // Thread manager reference
    ThreadManager* threadManager_;
    
    // Event queue
    std::vector<Event> eventQueue_;
    std::mutex queueMutex_;
    std::atomic<bool> stopProcessing_;
    std::atomic<bool> processingScheduled_;
    
    // Encryption state
    bool encryptionEnabled_;
    
    // Helper methods
    void invokeStringCallback(JNIEnv* env, const std::string& eventId, const std::string& data);
    void invokeIntCallback(JNIEnv* env, const std::string& eventId, int32_t data);
    void invokeFloatCallback(JNIEnv* env, const std::string& eventId, float data);
    void invokeDoubleCallback(JNIEnv* env, const std::string& eventId, double data);
    void invokeBooleanCallback(JNIEnv* env, const std::string& eventId, bool data);
    void invokeByteArrayCallback(JNIEnv* env, const std::string& eventId, const uint8_t* data, size_t length);
    
    // Helper to get JNIEnv for current thread
    JNIEnv* getJNIEnv();
};

#endif // IO_BRIDGE_H
