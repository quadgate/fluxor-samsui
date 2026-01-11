#include "socket_manager.h"
#include "thread_manager.h"
#include "io_bridge.h"
#include <android/log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

#define LOG_TAG "SocketManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096

SocketManager::SocketManager()
    : serverSocket_(-1),
      isRunning_(false),
      port_(0),
      threadManager_(nullptr),
      ioBridge_(nullptr),
      stopSending_(false) {
}

SocketManager::~SocketManager() {
    cleanup();
}

void SocketManager::setThreadManager(ThreadManager* threadManager) {
    threadManager_ = threadManager;
}

void SocketManager::setIOBridge(IOBridge* ioBridge) {
    ioBridge_ = ioBridge;
}

bool SocketManager::startServer(int port) {
    if (isRunning_.load()) {
        LOGE("Server already running");
        return false;
    }
    
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        LOGE("Failed to create socket: %s", strerror(errno));
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOGE("Failed to set socket options: %s", strerror(errno));
        close(serverSocket_);
        serverSocket_ = -1;
        return false;
    }
    
    // Bind socket
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        LOGE("Failed to bind socket: %s", strerror(errno));
        close(serverSocket_);
        serverSocket_ = -1;
        return false;
    }
    
    // Listen for connections
    if (listen(serverSocket_, MAX_CLIENTS) < 0) {
        LOGE("Failed to listen: %s", strerror(errno));
        close(serverSocket_);
        serverSocket_ = -1;
        return false;
    }
    
    port_ = port;
    isRunning_ = true;
    stopSending_ = false;
    
    // Start accept thread
    acceptThread_ = std::thread(&SocketManager::acceptConnections, this);
    
    // Start send worker thread
    sendThread_ = std::thread(&SocketManager::sendWorker, this);
    
    LOGI("TCP Server started on port %d", port);
    return true;
}

void SocketManager::stopServer() {
    if (!isRunning_.load()) {
        return;
    }
    
    isRunning_ = false;
    stopSending_ = true;
    
    // Close server socket
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
    
    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& client : clients_) {
            if (client && client->socketFd >= 0) {
                client->isConnected = false;
                close(client->socketFd);
            }
        }
        clients_.clear();
    }
    
    // Wake up send thread
    sendCondition_.notify_all();
    
    // Wait for threads to finish
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    if (sendThread_.joinable()) {
        sendThread_.join();
    }
    
    LOGI("TCP Server stopped");
}

bool SocketManager::isRunning() const {
    return isRunning_.load();
}

void SocketManager::sendToAllClients(const std::string& message) {
    if (!isRunning_.load()) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(sendQueueMutex_);
        sendQueue_.push(message); // Copy is intentional here - message may be used after call
    }
    sendCondition_.notify_one();
}

void SocketManager::sendWorker() {
    while (!stopSending_ || !sendQueue_.empty()) {
        std::unique_lock<std::mutex> lock(sendQueueMutex_);
        
        sendCondition_.wait(lock, [this] {
            return stopSending_ || !sendQueue_.empty();
        });
        
        if (stopSending_ && sendQueue_.empty()) {
            break;
        }
        
        if (sendQueue_.empty()) {
            continue;
        }
        
        std::string message = std::move(sendQueue_.front());
        sendQueue_.pop();
        lock.unlock();
        
        // Copy client list while holding lock, then release
        std::vector<int> clientSockets;
        {
            std::lock_guard<std::mutex> clientsLock(clientsMutex_);
            clientSockets.reserve(clients_.size());
            for (const auto& client : clients_) {
                if (client && client->isConnected.load() && client->socketFd >= 0) {
                    clientSockets.push_back(client->socketFd);
                }
            }
        }
        
        // Send to all clients outside the lock
        std::vector<int> toRemove;
        toRemove.reserve(clientSockets.size());
        
        // Pre-calculate message length
        uint32_t messageLen = htonl(static_cast<uint32_t>(message.length()));
        const char* messageData = message.c_str();
        size_t messageSize = message.length();
        
        for (int socketFd : clientSockets) {
            // Send message length prefix
            ssize_t sent = send(socketFd, &messageLen, sizeof(messageLen), 0);
            if (sent != sizeof(messageLen)) {
                toRemove.push_back(socketFd);
                continue;
            }
            
            // Send message
            size_t totalSent = 0;
            while (totalSent < messageSize) {
                sent = send(socketFd, messageData + totalSent, messageSize - totalSent, 0);
                if (sent < 0) {
                    toRemove.push_back(socketFd);
                    break;
                }
                totalSent += sent;
            }
        }
        
        // Remove disconnected clients (outside lock)
        for (int fd : toRemove) {
            removeClient(fd);
        }
    }
}

void SocketManager::acceptConnections() {
    LOGI("Accept thread started");
    
    while (isRunning_.load()) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddress, &clientAddressLen);
        
        if (clientSocket < 0) {
            if (isRunning_.load()) {
                LOGE("Accept failed: %s", strerror(errno));
            }
            break;
        }
        
        if (!isRunning_.load()) {
            close(clientSocket);
            break;
        }
        
        // Check client count
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            if (clients_.size() >= MAX_CLIENTS) {
                LOGE("Max clients reached, rejecting connection");
                close(clientSocket);
                continue;
            }
        }
        
        LOGI("New client connected: %s:%d", inet_ntoa(clientAddress.sin_addr), 
             ntohs(clientAddress.sin_port));
        
        // Create client connection
        auto client = std::make_unique<ClientConnection>(clientSocket);
        int socketFd = clientSocket;
        
        // Start handler thread
        client->handlerThread = std::thread(&SocketManager::handleClient, this, socketFd);
        
        // Store client
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clients_.push_back(std::move(client));
        }
        
        // Notify connection change
        notifyConnectionChange();
    }
    
    LOGI("Accept thread stopped");
}

void SocketManager::handleClient(int clientSocket) {
    std::vector<char> buffer(BUFFER_SIZE);
    
    while (isRunning_.load()) {
        // Read message length
        uint32_t messageLen;
        ssize_t received = recv(clientSocket, &messageLen, sizeof(messageLen), MSG_WAITALL);
        if (received != sizeof(messageLen)) {
            break;
        }
        
        messageLen = ntohl(messageLen);
        if (messageLen == 0 || messageLen > BUFFER_SIZE) {
            LOGE("Invalid message length: %u", messageLen);
            break;
        }
        
        // Resize buffer if needed
        if (buffer.size() < messageLen) {
            buffer.resize(messageLen);
        }
        
        // Read message
        size_t totalReceived = 0;
        while (totalReceived < messageLen) {
            received = recv(clientSocket, buffer.data() + totalReceived, 
                           messageLen - totalReceived, 0);
            if (received <= 0) {
                goto client_disconnected;
            }
            totalReceived += received;
        }
        
        // Create message string (avoid unnecessary null terminator)
        std::string message(buffer.data(), messageLen);
        
        LOGD("Received message from client %d: %s", clientSocket, message.c_str());
        
        // Forward to I/O bridge (use move to avoid copy)
        if (ioBridge_ != nullptr) {
            ioBridge_->postStringEvent("socket_message", std::move(message));
        }
    }
    
client_disconnected:
    LOGI("Client %d disconnected", clientSocket);
    removeClient(clientSocket);
}

void SocketManager::removeClient(int socketFd) {
    std::unique_ptr<ClientConnection> clientToRemove;
    
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            if ((*it) && (*it)->socketFd == socketFd) {
                (*it)->isConnected = false;
                clientToRemove = std::move(*it);
                clients_.erase(it);
                break;
            }
        }
    }
    
    // Cleanup outside lock
    if (clientToRemove) {
        if (clientToRemove->socketFd >= 0) {
            close(clientToRemove->socketFd);
        }
        if (clientToRemove->handlerThread.joinable()) {
            clientToRemove->handlerThread.join();
        }
        notifyConnectionChange();
    }
}

void SocketManager::notifyConnectionChange() {
    if (ioBridge_ != nullptr) {
        size_t count = getConnectedClientCount();
        ioBridge_->postIntEvent("socket_client_count", static_cast<int32_t>(count));
    }
}

size_t SocketManager::getConnectedClientCount() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    size_t count = 0;
    for (const auto& client : clients_) {
        if (client && client->isConnected.load()) {
            count++;
        }
    }
    return count;
}

void SocketManager::cleanup() {
    stopServer();
    threadManager_ = nullptr;
    ioBridge_ = nullptr;
}
