#include <jni.h>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <vector>
#include "thread_manager.h"
#include "io_bridge.h"
#include "socket_manager.h"

// Global thread manager instance
static ThreadManager* g_threadManager = nullptr;

// Global I/O bridge instance
static IOBridge* g_ioBridge = nullptr;

// Global socket manager instance
static SocketManager* g_socketManager = nullptr;

static JavaVM* g_jvm = nullptr;

// Get JVM reference
static void getJvmReference(JNIEnv* env) {
    if (g_jvm == nullptr && env != nullptr) {
        env->GetJavaVM(&g_jvm);
    }
}

// Initialize thread manager
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_initThreadManager(JNIEnv* env, jobject /* this */) {
    if (g_threadManager == nullptr) {
        g_threadManager = new ThreadManager();
        // Initialize thread pool - use optimal size based on CPU cores
        unsigned int threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) {
            threadCount = 4; // Fallback to 4 if hardware_concurrency() returns 0
        } else {
            // Use number of cores + 1 for I/O bound tasks
            threadCount = std::min(threadCount + 1, 8u); // Cap at 8 threads
        }
        g_threadManager->initializeThreadPool(threadCount);
    }
    getJvmReference(env);
}

// Cleanup thread manager
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_cleanupThreadManager(JNIEnv* env, jobject /* this */) {
    if (g_threadManager != nullptr) {
        delete g_threadManager;
        g_threadManager = nullptr;
    }
}

// Create a new thread
extern "C" JNIEXPORT jlong JNICALL
Java_com_fluxorio_MainActivity_createThread(JNIEnv* env, jobject /* this */, jstring name) {
    if (g_threadManager == nullptr) {
        return -1;
    }
    
    const char* nameStr = env->GetStringUTFChars(name, nullptr);
    std::string threadName = nameStr ? nameStr : "";
    env->ReleaseStringUTFChars(name, nameStr);
    
    size_t threadIndex = g_threadManager->createThread(threadName, []() {
        // Default task - can be customized
    });
    
    return static_cast<jlong>(threadIndex);
}

// Join a thread
extern "C" JNIEXPORT jboolean JNICALL
Java_com_fluxorio_MainActivity_joinThread(JNIEnv* env, jobject /* this */, jlong threadIndex) {
    if (g_threadManager == nullptr) {
        return JNI_FALSE;
    }
    
    bool result = g_threadManager->joinThread(static_cast<size_t>(threadIndex));
    return result ? JNI_TRUE : JNI_FALSE;
}

// Detach a thread
extern "C" JNIEXPORT jboolean JNICALL
Java_com_fluxorio_MainActivity_detachThread(JNIEnv* env, jobject /* this */, jlong threadIndex) {
    if (g_threadManager == nullptr) {
        return JNI_FALSE;
    }
    
    bool result = g_threadManager->detachThread(static_cast<size_t>(threadIndex));
    return result ? JNI_TRUE : JNI_FALSE;
}

// Get active thread count
extern "C" JNIEXPORT jint JNICALL
Java_com_fluxorio_MainActivity_getActiveThreadCount(JNIEnv* env, jobject /* this */) {
    if (g_threadManager == nullptr) {
        return 0;
    }
    
    return static_cast<jint>(g_threadManager->getActiveThreadCount());
}

// Get total thread count
extern "C" JNIEXPORT jint JNICALL
Java_com_fluxorio_MainActivity_getTotalThreadCount(JNIEnv* env, jobject /* this */) {
    if (g_threadManager == nullptr) {
        return 0;
    }
    
    return static_cast<jint>(g_threadManager->getTotalThreadCount());
}

// Initialize thread pool
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_initThreadPool(JNIEnv* env, jobject /* this */, jint poolSize) {
    if (g_threadManager == nullptr) {
        return;
    }
    
    g_threadManager->initializeThreadPool(static_cast<size_t>(poolSize));
}

// Shutdown thread pool
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_shutdownThreadPool(JNIEnv* env, jobject /* this */) {
    if (g_threadManager == nullptr) {
        return;
    }
    
    g_threadManager->shutdownThreadPool();
}

// Initialize I/O bridge
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_initIOBridge(JNIEnv* env, jobject /* this */) {
    getJvmReference(env);
    
    if (g_ioBridge == nullptr && g_jvm != nullptr) {
        g_ioBridge = new IOBridge();
        g_ioBridge->initialize(g_jvm);
        
        // Set thread manager reference
        if (g_threadManager != nullptr) {
            g_ioBridge->setThreadManager(g_threadManager);
        }
        
        // Set I/O bridge reference in socket manager if already initialized
        if (g_socketManager != nullptr) {
            g_socketManager->setIOBridge(g_ioBridge);
        }
    }
}

// Cleanup I/O bridge
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_cleanupIOBridge(JNIEnv* env, jobject /* this */) {
    if (g_ioBridge != nullptr) {
        g_ioBridge->cleanup();
        delete g_ioBridge;
        g_ioBridge = nullptr;
    }
}

// Register listener for I/O bridge
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_registerIOBridgeListener(JNIEnv* env, jobject /* this */, jobject listener) {
    if (g_ioBridge == nullptr || env == nullptr || listener == nullptr) {
        return;
    }
    
    g_ioBridge->registerListener(env, listener);
}

// Unregister listener for I/O bridge
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_unregisterIOBridgeListener(JNIEnv* env, jobject /* this */) {
    if (g_ioBridge == nullptr || env == nullptr) {
        return;
    }
    
    g_ioBridge->unregisterListener(env);
}

// Post string event from C++ to Kotlin
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_postStringEvent(JNIEnv* env, jobject /* this */, jstring eventId, jstring data) {
    if (g_ioBridge == nullptr || env == nullptr) {
        return;
    }
    
    const char* eventIdStr = env->GetStringUTFChars(eventId, nullptr);
    const char* dataStr = env->GetStringUTFChars(data, nullptr);
    
    if (eventIdStr && dataStr) {
        std::string eventIdCpp = eventIdStr;
        std::string dataCpp = dataStr;
        g_ioBridge->postStringEvent(eventIdCpp, dataCpp);
    }
    
    if (eventIdStr) {
        env->ReleaseStringUTFChars(eventId, eventIdStr);
    }
    if (dataStr) {
        env->ReleaseStringUTFChars(data, dataStr);
    }
}

// Post integer event from C++ to Kotlin
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_postIntEvent(JNIEnv* env, jobject /* this */, jstring eventId, jint data) {
    if (g_ioBridge == nullptr || env == nullptr) {
        return;
    }
    
    const char* eventIdStr = env->GetStringUTFChars(eventId, nullptr);
    if (eventIdStr) {
        std::string eventIdCpp = eventIdStr;
        g_ioBridge->postIntEvent(eventIdCpp, static_cast<int32_t>(data));
        env->ReleaseStringUTFChars(eventId, eventIdStr);
    }
}

// Post float event from C++ to Kotlin
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_postFloatEvent(JNIEnv* env, jobject /* this */, jstring eventId, jfloat data) {
    if (g_ioBridge == nullptr || env == nullptr) {
        return;
    }
    
    const char* eventIdStr = env->GetStringUTFChars(eventId, nullptr);
    if (eventIdStr) {
        std::string eventIdCpp = eventIdStr;
        g_ioBridge->postFloatEvent(eventIdCpp, data);
        env->ReleaseStringUTFChars(eventId, eventIdStr);
    }
}

// Post double event from C++ to Kotlin
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_postDoubleEvent(JNIEnv* env, jobject /* this */, jstring eventId, jdouble data) {
    if (g_ioBridge == nullptr || env == nullptr) {
        return;
    }
    
    const char* eventIdStr = env->GetStringUTFChars(eventId, nullptr);
    if (eventIdStr) {
        std::string eventIdCpp = eventIdStr;
        g_ioBridge->postDoubleEvent(eventIdCpp, data);
        env->ReleaseStringUTFChars(eventId, eventIdStr);
    }
}

// Post boolean event from C++ to Kotlin
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_postBooleanEvent(JNIEnv* env, jobject /* this */, jstring eventId, jboolean data) {
    if (g_ioBridge == nullptr || env == nullptr) {
        return;
    }
    
    const char* eventIdStr = env->GetStringUTFChars(eventId, nullptr);
    if (eventIdStr) {
        std::string eventIdCpp = eventIdStr;
        g_ioBridge->postBooleanEvent(eventIdCpp, data == JNI_TRUE);
        env->ReleaseStringUTFChars(eventId, eventIdStr);
    }
}

// Post byte array event from C++ to Kotlin
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_postByteArrayEvent(JNIEnv* env, jobject /* this */, jstring eventId, jbyteArray data) {
    if (g_ioBridge == nullptr || env == nullptr) {
        return;
    }
    
    const char* eventIdStr = env->GetStringUTFChars(eventId, nullptr);
    if (eventIdStr && data != nullptr) {
        jsize length = env->GetArrayLength(data);
        jbyte* bytes = env->GetByteArrayElements(data, nullptr);
        
        if (bytes != nullptr) {
            std::string eventIdCpp = eventIdStr;
            g_ioBridge->postByteArrayEvent(eventIdCpp, reinterpret_cast<uint8_t*>(bytes), static_cast<size_t>(length));
            env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
        }
        env->ReleaseStringUTFChars(eventId, eventIdStr);
    }
}

// Initialize socket manager
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_initSocketManager(JNIEnv* env, jobject /* this */) {
    if (g_socketManager == nullptr) {
        g_socketManager = new SocketManager();
        
        if (g_threadManager != nullptr) {
            g_socketManager->setThreadManager(g_threadManager);
        }
        
        if (g_ioBridge != nullptr) {
            g_socketManager->setIOBridge(g_ioBridge);
        }
    }
}

// Cleanup socket manager
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_cleanupSocketManager(JNIEnv* env, jobject /* this */) {
    if (g_socketManager != nullptr) {
        g_socketManager->cleanup();
        delete g_socketManager;
        g_socketManager = nullptr;
    }
}

// Start socket server
extern "C" JNIEXPORT jboolean JNICALL
Java_com_fluxorio_MainActivity_startSocketServer(JNIEnv* env, jobject /* this */, jint port) {
    if (g_socketManager == nullptr) {
        return JNI_FALSE;
    }
    
    bool result = g_socketManager->startServer(static_cast<int>(port));
    return result ? JNI_TRUE : JNI_FALSE;
}

// Stop socket server
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_stopSocketServer(JNIEnv* env, jobject /* this */) {
    if (g_socketManager == nullptr) {
        return;
    }
    
    g_socketManager->stopServer();
}

// Send message to all connected clients
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_sendMessageToClients(JNIEnv* env, jobject /* this */, jstring message) {
    if (g_socketManager == nullptr || env == nullptr || message == nullptr) {
        return;
    }
    
    const char* messageStr = env->GetStringUTFChars(message, nullptr);
    if (!messageStr) {
        return;
    }
    
    std::string messageCpp = messageStr;
    env->ReleaseStringUTFChars(message, messageStr);
    
    g_socketManager->sendToAllClients(messageCpp);
}

// Get connected client count
extern "C" JNIEXPORT jint JNICALL
Java_com_fluxorio_MainActivity_getConnectedClientCount(JNIEnv* env, jobject /* this */) {
    if (g_socketManager == nullptr) {
        return 0;
    }
    
    return static_cast<jint>(g_socketManager->getConnectedClientCount());
}

// Send message to thread handler - processes in background thread and sends back via I/O bridge
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_sendMessageToThreadHandler(JNIEnv* env, jobject /* this */, jstring message) {
    if (g_threadManager == nullptr || g_ioBridge == nullptr || env == nullptr || message == nullptr) {
        return;
    }
    
    const char* messageStr = env->GetStringUTFChars(message, nullptr);
    if (!messageStr) {
        return;
    }
    
    std::string messageCpp = messageStr;
    env->ReleaseStringUTFChars(message, messageStr);
    
    // Submit task to thread pool for processing
    g_threadManager->submitTask([messageCpp]() {
        // Simulate processing (e.g., CPU-intensive work, network I/O, etc.)
        // In a real app, this could be image processing, data analysis, etc.
        
        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Process the message (example: echo it back with processing info)
        std::string processedMessage = "[Processed] " + messageCpp + " (handled by C++ thread)";
        
        // Send processed message back to Kotlin via I/O bridge
        if (g_ioBridge != nullptr) {
            g_ioBridge->postStringEvent("message_response", processedMessage);
        }
    });
}

// Send image to thread handler - processes in background thread and sends back via I/O bridge
extern "C" JNIEXPORT void JNICALL
Java_com_fluxorio_MainActivity_sendImageToThreadHandler(JNIEnv* env, jobject /* this */, jbyteArray imageData) {
    if (g_threadManager == nullptr || g_ioBridge == nullptr || env == nullptr || imageData == nullptr) {
        return;
    }
    
    // Get image data length
    jsize length = env->GetArrayLength(imageData);
    if (length <= 0) {
        return;
    }
    
    // Get byte array elements
    jbyte* bytes = env->GetByteArrayElements(imageData, nullptr);
    if (bytes == nullptr) {
        return;
    }
    
    // Copy image data to C++ vector
    std::vector<uint8_t> imageDataCpp(reinterpret_cast<uint8_t*>(bytes), reinterpret_cast<uint8_t*>(bytes) + length);
    
    // Release byte array elements
    env->ReleaseByteArrayElements(imageData, bytes, JNI_ABORT);
    
    // Submit task to thread pool for processing
    g_threadManager->submitTask([imageDataCpp]() {
        // Simulate image processing (e.g., resize, filter, analyze, etc.)
        // In a real app, this could be actual image processing using OpenCV, image libraries, etc.
        
        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Process the image (example: echo back processed image data or analysis results)
        // For demonstration, we'll return a processed version (in real app, do actual processing)
        std::vector<uint8_t> processedImage = imageDataCpp; // In real app, process the image here
        
        // You could also send back metadata as string
        std::string imageInfo = "Image processed: " + std::to_string(imageDataCpp.size()) + " bytes";
        
        // Send processed image data back to Kotlin via I/O bridge
        if (g_ioBridge != nullptr) {
            g_ioBridge->postByteArrayEvent("image_response", processedImage.data(), processedImage.size());
            
            // Also send info as string
            g_ioBridge->postStringEvent("image_info", imageInfo);
        }
    });
}

// Original stringFromJNI function
extern "C" JNIEXPORT jstring JNICALL
Java_com_fluxorio_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}