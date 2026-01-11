#ifndef SOCKET_MANAGER_H
#define SOCKET_MANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <queue>
#include <memory>
#include <cstdint>

// Forward declarations
class ThreadManager;
class IOBridge;

struct ClientConnection {
    int socketFd;
    std::thread handlerThread;
    std::atomic<bool> isConnected;
    
    ClientConnection(int fd) : socketFd(fd), isConnected(true) {}
    
    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(const ClientConnection&) = delete;
    
    ClientConnection(ClientConnection&& other) noexcept
        : socketFd(other.socketFd),
          handlerThread(std::move(other.handlerThread)),
          isConnected(other.isConnected.load()) {
        other.socketFd = -1;
        other.isConnected = false;
    }
};

class SocketManager {
public:
    SocketManager();
    ~SocketManager();
    
    // Disable copy constructor and assignment operator
    SocketManager(const SocketManager&) = delete;
    SocketManager& operator=(const SocketManager&) = delete;
    
    // Initialization
    void setThreadManager(ThreadManager* threadManager);
    void setIOBridge(IOBridge* ioBridge);
    
    // Server control
    bool startServer(int port);
    void stopServer();
    bool isRunning() const;
    
    // Message sending
    void sendToAllClients(const std::string& message);
    
    // Client management
    size_t getConnectedClientCount() const;
    
    // Cleanup
    void cleanup();

private:
    // Server socket
    int serverSocket_;
    std::atomic<bool> isRunning_;
    std::atomic<int> port_;
    
    // Client management
    std::vector<std::unique_ptr<ClientConnection>> clients_;
    mutable std::mutex clientsMutex_;
    
    // Message queue for sending
    std::queue<std::string> sendQueue_;
    std::mutex sendQueueMutex_;
    std::condition_variable sendCondition_;
    std::atomic<bool> stopSending_;
    
    // References
    ThreadManager* threadManager_;
    IOBridge* ioBridge_;
    
    // Threads
    std::thread acceptThread_;
    std::thread sendThread_;
    
    // Helper methods
    void acceptConnections();
    void handleClient(int clientSocket);
    void sendWorker();
    void removeClient(int socketFd);
    void notifyConnectionChange();
};

#endif // SOCKET_MANAGER_H
