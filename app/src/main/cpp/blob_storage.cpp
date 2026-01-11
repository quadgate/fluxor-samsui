#include "blob_storage.h"
#include <fstream>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <cerrno>
#include <android/log.h>

#define LOG_TAG "BlobStorage"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

BlobStorage::BlobStorage() {
}

BlobStorage::~BlobStorage() {
}

bool BlobStorage::ensureDirectoryExists(const std::string& filePath) {
    // Extract directory path from file path
    size_t lastSlash = filePath.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return false; // Invalid path
    }
    
    std::string dirPath = filePath.substr(0, lastSlash);
    
    // Check if directory already exists
    struct stat info;
    if (stat(dirPath.c_str(), &info) == 0 && S_ISDIR(info.st_mode)) {
        return true; // Directory exists
    }
    
    // Create directory (and parent directories if needed)
    // Android uses mode 0755 for app directories
    mode_t mode = 0755;
    
    // Create parent directories recursively
    size_t pos = 0;
    while ((pos = dirPath.find('/', pos + 1)) != std::string::npos) {
        std::string parentDir = dirPath.substr(0, pos);
        if (stat(parentDir.c_str(), &info) != 0) {
            if (mkdir(parentDir.c_str(), mode) != 0 && errno != EEXIST) {
                LOGE("Failed to create directory: %s", parentDir.c_str());
                return false;
            }
        }
    }
    
    // Create the final directory
    if (mkdir(dirPath.c_str(), mode) != 0 && errno != EEXIST) {
        LOGE("Failed to create directory: %s", dirPath.c_str());
        return false;
    }
    
    return true;
}

bool BlobStorage::saveMessages(const std::string& filePath, const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
        LOGE("Invalid data for saveMessages");
        return false;
    }
    
    // Ensure directory exists
    if (!ensureDirectoryExists(filePath)) {
        LOGE("Failed to ensure directory exists for: %s", filePath.c_str());
        return false;
    }
    
    // Open file for writing in binary mode
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        LOGE("Failed to open file for writing: %s", filePath.c_str());
        return false;
    }
    
    // Write data
    file.write(reinterpret_cast<const char*>(data), length);
    
    if (!file.good()) {
        LOGE("Failed to write data to file: %s", filePath.c_str());
        file.close();
        return false;
    }
    
    file.close();
    return true;
}

bool BlobStorage::loadMessages(const std::string& filePath, std::vector<uint8_t>& data) {
    // Check if file exists and is readable
    struct stat info;
    if (stat(filePath.c_str(), &info) != 0) {
        // File doesn't exist
        data.clear();
        return true; // Not an error, just empty
    }
    
    if (!S_ISREG(info.st_mode)) {
        LOGE("Path is not a regular file: %s", filePath.c_str());
        return false;
    }
    
    // Open file for reading in binary mode
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOGE("Failed to open file for reading: %s", filePath.c_str());
        return false;
    }
    
    // Get file size
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (fileSize <= 0) {
        data.clear();
        file.close();
        return true; // Empty file is valid
    }
    
    // Read file content
    data.resize(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    
    if (!file.good() && !file.eof()) {
        LOGE("Failed to read data from file: %s", filePath.c_str());
        file.close();
        return false;
    }
    
    file.close();
    return true;
}

bool BlobStorage::clearMessages(const std::string& filePath) {
    // Check if file exists
    struct stat info;
    if (stat(filePath.c_str(), &info) != 0) {
        return true; // File doesn't exist, nothing to clear
    }
    
    // Delete file
    if (unlink(filePath.c_str()) != 0) {
        LOGE("Failed to delete file: %s", filePath.c_str());
        return false;
    }
    
    return true;
}

bool BlobStorage::hasMessages(const std::string& filePath) {
    struct stat info;
    if (stat(filePath.c_str(), &info) != 0) {
        return false; // File doesn't exist
    }
    
    if (!S_ISREG(info.st_mode)) {
        return false; // Not a regular file
    }
    
    return info.st_size > 0;
}

int64_t BlobStorage::getStorageSize(const std::string& filePath) {
    struct stat info;
    if (stat(filePath.c_str(), &info) != 0) {
        return 0; // File doesn't exist
    }
    
    if (!S_ISREG(info.st_mode)) {
        return 0; // Not a regular file
    }
    
    return static_cast<int64_t>(info.st_size);
}
