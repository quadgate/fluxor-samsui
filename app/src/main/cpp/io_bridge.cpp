#include "io_bridge.h"
#include "thread_manager.h"
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "IOBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

IOBridge::IOBridge()
    : jvm_(nullptr),
      listenerObject_(nullptr),
      listenerClass_(nullptr),
      threadManager_(nullptr),
      stopProcessing_(false),
      processingScheduled_(false) {
}

IOBridge::~IOBridge() {
    cleanup();
}

void IOBridge::initialize(JavaVM* jvm) {
    if (jvm_ != nullptr) {
        return; // Already initialized
    }
    
    jvm_ = jvm;
    stopProcessing_ = false;
    LOGI("IOBridge initialized");
}

void IOBridge::cleanup() {
    if (jvm_ != nullptr && listenerObject_ != nullptr) {
        JNIEnv* env = getJNIEnv();
        if (env != nullptr) {
            env->DeleteGlobalRef(listenerObject_);
            listenerObject_ = nullptr;
        }
        if (listenerClass_ != nullptr) {
            env->DeleteGlobalRef(listenerClass_);
            listenerClass_ = nullptr;
        }
    }
    
    stopProcessing_ = true;
    processingScheduled_ = false;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.clear();
    }
    
    jvm_ = nullptr;
    LOGI("IOBridge cleaned up");
}

void IOBridge::registerListener(JNIEnv* env, jobject listener) {
    if (jvm_ == nullptr || env == nullptr) {
        LOGE("Cannot register listener: bridge not initialized");
        return;
    }
    
    // Clean up existing listener if any
    unregisterListener(env);
    
    // Create global reference to listener object
    listenerObject_ = env->NewGlobalRef(listener);
    if (listenerObject_ == nullptr) {
        LOGE("Failed to create global reference for listener");
        return;
    }
    
    // Get listener class and create global reference
    jclass localClass = env->GetObjectClass(listener);
    if (localClass == nullptr) {
        LOGE("Failed to get listener class");
        env->DeleteGlobalRef(listenerObject_);
        listenerObject_ = nullptr;
        return;
    }
    
    listenerClass_ = static_cast<jclass>(env->NewGlobalRef(localClass));
    env->DeleteLocalRef(localClass);
    
    if (listenerClass_ == nullptr) {
        LOGE("Failed to create global reference for listener class");
        env->DeleteGlobalRef(listenerObject_);
        listenerObject_ = nullptr;
        return;
    }
    
    LOGI("Listener registered successfully");
}

void IOBridge::unregisterListener(JNIEnv* env) {
    if (env == nullptr) {
        return;
    }
    
    if (listenerObject_ != nullptr) {
        env->DeleteGlobalRef(listenerObject_);
        listenerObject_ = nullptr;
    }
    
    if (listenerClass_ != nullptr) {
        env->DeleteGlobalRef(listenerClass_);
        listenerClass_ = nullptr;
    }
    
    LOGI("Listener unregistered");
}

void IOBridge::postStringEvent(const std::string& eventId, const std::string& data) {
    if (!isInitialized()) {
        LOGE("Cannot post event: bridge not initialized");
        return;
    }
    
    Event event;
    event.type = EventType::STRING;
    event.eventId = eventId;
    event.stringValue = data;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.push_back(std::move(event));
    }
    
    // Submit to thread pool for processing (only if not already scheduled)
    if (threadManager_ != nullptr) {
        bool expected = false;
        if (processingScheduled_.compare_exchange_strong(expected, true)) {
            threadManager_->submitTask([this]() {
                processEvents();
                processingScheduled_ = false;
            });
        }
    }
}

void IOBridge::postIntEvent(const std::string& eventId, int32_t data) {
    if (!isInitialized()) {
        LOGE("Cannot post event: bridge not initialized");
        return;
    }
    
    Event event;
    event.type = EventType::INT;
    event.eventId = eventId;
    event.intValue = data;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.push_back(std::move(event));
    }
    
    if (threadManager_ != nullptr) {
        bool expected = false;
        if (processingScheduled_.compare_exchange_strong(expected, true)) {
            threadManager_->submitTask([this]() {
                processEvents();
                processingScheduled_ = false;
            });
        }
    }
}

void IOBridge::postFloatEvent(const std::string& eventId, float data) {
    if (!isInitialized()) {
        LOGE("Cannot post event: bridge not initialized");
        return;
    }
    
    Event event;
    event.type = EventType::FLOAT;
    event.eventId = eventId;
    event.floatValue = data;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.push_back(std::move(event));
    }
    
    if (threadManager_ != nullptr) {
        bool expected = false;
        if (processingScheduled_.compare_exchange_strong(expected, true)) {
            threadManager_->submitTask([this]() {
                processEvents();
                processingScheduled_ = false;
            });
        }
    }
}

void IOBridge::postDoubleEvent(const std::string& eventId, double data) {
    if (!isInitialized()) {
        LOGE("Cannot post event: bridge not initialized");
        return;
    }
    
    Event event;
    event.type = EventType::DOUBLE;
    event.eventId = eventId;
    event.doubleValue = data;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.push_back(std::move(event));
    }
    
    if (threadManager_ != nullptr) {
        bool expected = false;
        if (processingScheduled_.compare_exchange_strong(expected, true)) {
            threadManager_->submitTask([this]() {
                processEvents();
                processingScheduled_ = false;
            });
        }
    }
}

void IOBridge::postBooleanEvent(const std::string& eventId, bool data) {
    if (!isInitialized()) {
        LOGE("Cannot post event: bridge not initialized");
        return;
    }
    
    Event event;
    event.type = EventType::BOOLEAN;
    event.eventId = eventId;
    event.boolValue = data;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.push_back(std::move(event));
    }
    
    if (threadManager_ != nullptr) {
        bool expected = false;
        if (processingScheduled_.compare_exchange_strong(expected, true)) {
            threadManager_->submitTask([this]() {
                processEvents();
                processingScheduled_ = false;
            });
        }
    }
}

void IOBridge::postByteArrayEvent(const std::string& eventId, const uint8_t* data, size_t length) {
    if (!isInitialized()) {
        LOGE("Cannot post event: bridge not initialized");
        return;
    }
    
    Event event;
    event.type = EventType::BYTE_ARRAY;
    event.eventId = eventId;
    event.byteArrayValue.assign(data, data + length);
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.push_back(std::move(event));
    }
    
    if (threadManager_ != nullptr) {
        bool expected = false;
        if (processingScheduled_.compare_exchange_strong(expected, true)) {
            threadManager_->submitTask([this]() {
                processEvents();
                processingScheduled_ = false;
            });
        }
    }
}

void IOBridge::processEvents() {
    if (jvm_ == nullptr || listenerObject_ == nullptr) {
        return;
    }
    
    JNIEnv* env = getJNIEnv();
    if (env == nullptr) {
        LOGE("Failed to get JNIEnv for event processing");
        return;
    }
    
    std::vector<Event> eventsToProcess;
    
    // Move events from queue (more efficient than copying)
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventsToProcess.reserve(eventQueue_.size());
        eventsToProcess = std::move(eventQueue_);
        eventQueue_.clear();
    }
    
    // Process each event
    for (const auto& event : eventsToProcess) {
        jstring eventIdStr = env->NewStringUTF(event.eventId.c_str());
        
        switch (event.type) {
            case EventType::STRING:
                invokeStringCallback(env, event.eventId, event.stringValue);
                break;
            case EventType::INT:
                invokeIntCallback(env, event.eventId, event.intValue);
                break;
            case EventType::FLOAT:
                invokeFloatCallback(env, event.eventId, event.floatValue);
                break;
            case EventType::DOUBLE:
                invokeDoubleCallback(env, event.eventId, event.doubleValue);
                break;
            case EventType::BOOLEAN:
                invokeBooleanCallback(env, event.eventId, event.boolValue);
                break;
            case EventType::BYTE_ARRAY:
                invokeByteArrayCallback(env, event.eventId, event.byteArrayValue.data(), event.byteArrayValue.size());
                break;
        }
        
        if (eventIdStr != nullptr) {
            env->DeleteLocalRef(eventIdStr);
        }
        
        // Check for exceptions
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }
}

void IOBridge::setThreadManager(ThreadManager* threadManager) {
    threadManager_ = threadManager;
}

bool IOBridge::isInitialized() const {
    return jvm_ != nullptr && listenerObject_ != nullptr;
}

JNIEnv* IOBridge::getJNIEnv() {
    if (jvm_ == nullptr) {
        return nullptr;
    }
    
    JNIEnv* env = nullptr;
    jint result = jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    
    if (result == JNI_EDETACHED) {
        // Thread not attached, attach it
        result = jvm_->AttachCurrentThread(&env, nullptr);
        if (result != JNI_OK) {
            LOGE("Failed to attach thread to JVM");
            return nullptr;
        }
    } else if (result != JNI_OK) {
        LOGE("Failed to get JNIEnv");
        return nullptr;
    }
    
    return env;
}

void IOBridge::invokeStringCallback(JNIEnv* env, const std::string& eventId, const std::string& data) {
    if (listenerClass_ == nullptr || listenerObject_ == nullptr) {
        return;
    }
    
    jmethodID methodId = env->GetMethodID(listenerClass_, "onStringEvent", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (methodId == nullptr) {
        LOGE("Failed to find onStringEvent method");
        return;
    }
    
    jstring eventIdStr = env->NewStringUTF(eventId.c_str());
    jstring dataStr = env->NewStringUTF(data.c_str());
    
    env->CallVoidMethod(listenerObject_, methodId, eventIdStr, dataStr);
    
    if (eventIdStr != nullptr) {
        env->DeleteLocalRef(eventIdStr);
    }
    if (dataStr != nullptr) {
        env->DeleteLocalRef(dataStr);
    }
}

void IOBridge::invokeIntCallback(JNIEnv* env, const std::string& eventId, int32_t data) {
    if (listenerClass_ == nullptr || listenerObject_ == nullptr) {
        return;
    }
    
    jmethodID methodId = env->GetMethodID(listenerClass_, "onIntEvent", "(Ljava/lang/String;I)V");
    if (methodId == nullptr) {
        LOGE("Failed to find onIntEvent method");
        return;
    }
    
    jstring eventIdStr = env->NewStringUTF(eventId.c_str());
    env->CallVoidMethod(listenerObject_, methodId, eventIdStr, data);
    
    if (eventIdStr != nullptr) {
        env->DeleteLocalRef(eventIdStr);
    }
}

void IOBridge::invokeFloatCallback(JNIEnv* env, const std::string& eventId, float data) {
    if (listenerClass_ == nullptr || listenerObject_ == nullptr) {
        return;
    }
    
    jmethodID methodId = env->GetMethodID(listenerClass_, "onFloatEvent", "(Ljava/lang/String;F)V");
    if (methodId == nullptr) {
        LOGE("Failed to find onFloatEvent method");
        return;
    }
    
    jstring eventIdStr = env->NewStringUTF(eventId.c_str());
    env->CallVoidMethod(listenerObject_, methodId, eventIdStr, data);
    
    if (eventIdStr != nullptr) {
        env->DeleteLocalRef(eventIdStr);
    }
}

void IOBridge::invokeDoubleCallback(JNIEnv* env, const std::string& eventId, double data) {
    if (listenerClass_ == nullptr || listenerObject_ == nullptr) {
        return;
    }
    
    jmethodID methodId = env->GetMethodID(listenerClass_, "onDoubleEvent", "(Ljava/lang/String;D)V");
    if (methodId == nullptr) {
        LOGE("Failed to find onDoubleEvent method");
        return;
    }
    
    jstring eventIdStr = env->NewStringUTF(eventId.c_str());
    env->CallVoidMethod(listenerObject_, methodId, eventIdStr, data);
    
    if (eventIdStr != nullptr) {
        env->DeleteLocalRef(eventIdStr);
    }
}

void IOBridge::invokeBooleanCallback(JNIEnv* env, const std::string& eventId, bool data) {
    if (listenerClass_ == nullptr || listenerObject_ == nullptr) {
        return;
    }
    
    jmethodID methodId = env->GetMethodID(listenerClass_, "onBooleanEvent", "(Ljava/lang/String;Z)V");
    if (methodId == nullptr) {
        LOGE("Failed to find onBooleanEvent method");
        return;
    }
    
    jstring eventIdStr = env->NewStringUTF(eventId.c_str());
    env->CallVoidMethod(listenerObject_, methodId, eventIdStr, data ? JNI_TRUE : JNI_FALSE);
    
    if (eventIdStr != nullptr) {
        env->DeleteLocalRef(eventIdStr);
    }
}

void IOBridge::invokeByteArrayCallback(JNIEnv* env, const std::string& eventId, const uint8_t* data, size_t length) {
    if (listenerClass_ == nullptr || listenerObject_ == nullptr) {
        return;
    }
    
    jmethodID methodId = env->GetMethodID(listenerClass_, "onByteArrayEvent", "(Ljava/lang/String;[B)V");
    if (methodId == nullptr) {
        LOGE("Failed to find onByteArrayEvent method");
        return;
    }
    
    jstring eventIdStr = env->NewStringUTF(eventId.c_str());
    jbyteArray byteArray = env->NewByteArray(static_cast<jsize>(length));
    
    if (byteArray != nullptr) {
        env->SetByteArrayRegion(byteArray, 0, static_cast<jsize>(length), reinterpret_cast<const jbyte*>(data));
        env->CallVoidMethod(listenerObject_, methodId, eventIdStr, byteArray);
        env->DeleteLocalRef(byteArray);
    }
    
    if (eventIdStr != nullptr) {
        env->DeleteLocalRef(eventIdStr);
    }
}
