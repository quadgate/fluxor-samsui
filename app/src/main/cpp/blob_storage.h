#ifndef BLOB_STORAGE_H
#define BLOB_STORAGE_H

#include <string>
#include <vector>
#include <cstdint>

/**
 * BlobStorage - Handles local persistence of messages using binary file storage (C++ implementation)
 */
class BlobStorage {
public:
    BlobStorage();
    ~BlobStorage();
    
    // Disable copy constructor and assignment operator
    BlobStorage(const BlobStorage&) = delete;
    BlobStorage& operator=(const BlobStorage&) = delete;
    
    /**
     * Save messages to blob storage
     * @param filePath Full path to the storage file
     * @param data Serialized message data (binary format)
     * @param length Length of data in bytes
     * @return true on success, false on error
     */
    bool saveMessages(const std::string& filePath, const uint8_t* data, size_t length);
    
    /**
     * Load messages from blob storage
     * @param filePath Full path to the storage file
     * @param data Output buffer for serialized message data
     * @return true on success, false on error
     */
    bool loadMessages(const std::string& filePath, std::vector<uint8_t>& data);
    
    /**
     * Clear all stored messages
     * @param filePath Full path to the storage file
     * @return true on success, false on error
     */
    bool clearMessages(const std::string& filePath);
    
    /**
     * Check if messages exist in storage
     * @param filePath Full path to the storage file
     * @return true if file exists and has size > 0, false otherwise
     */
    bool hasMessages(const std::string& filePath);
    
    /**
     * Get storage file size in bytes
     * @param filePath Full path to the storage file
     * @return File size in bytes, or 0 if file doesn't exist
     */
    int64_t getStorageSize(const std::string& filePath);
    
private:
    /**
     * Ensure directory exists for the given file path
     * @param filePath Full path to the file
     * @return true on success, false on error
     */
    bool ensureDirectoryExists(const std::string& filePath);
};

#endif // BLOB_STORAGE_H
