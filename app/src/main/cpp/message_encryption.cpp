#include "message_encryption.h"
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace {
    // Encryption key (in production, this should be securely stored/derived)
    const std::string ENCRYPTION_KEY = "FluxorSecretKey2024!";
    
    // Base64 encoding characters
    const char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    // Simple base64 encoding
    std::string base64Encode(const std::string& input) {
        std::string output;
        int val = 0;
        int valb = -6;
        
        for (unsigned char c : input) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                output.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        
        if (valb > -6) {
            output.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
        }
        
        while (output.size() % 4) {
            output.push_back('=');
        }
        
        return output;
    }
    
    // Simple base64 decoding
    std::string base64Decode(const std::string& input) {
        std::string output;
        int val = 0;
        int valb = -8;
        
        for (char c : input) {
            if (c == '=') break;
            
            size_t pos = 0;
            bool found = false;
            for (size_t i = 0; i < 64; ++i) {
                if (BASE64_CHARS[i] == c) {
                    pos = i;
                    found = true;
                    break;
                }
            }
            if (!found) continue; // Skip invalid characters
            
            val = (val << 6) + static_cast<int>(pos);
            valb += 6;
            if (valb >= 0) {
                output.push_back(static_cast<char>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        
        return output;
    }
}

std::string encryptMessage(const std::string& message) {
    if (message.empty()) {
        return message;
    }
    
    // XOR encryption with key
    std::string encrypted;
    encrypted.reserve(message.size());
    
    for (size_t i = 0; i < message.size(); ++i) {
        char keyChar = ENCRYPTION_KEY[i % ENCRYPTION_KEY.size()];
        encrypted.push_back(message[i] ^ keyChar);
    }
    
    // Encode to base64 for safe storage/transmission
    return base64Encode(encrypted);
}

std::string decryptMessage(const std::string& encryptedMessage) {
    if (encryptedMessage.empty()) {
        return encryptedMessage;
    }
    
    // Decode from base64
    std::string encrypted = base64Decode(encryptedMessage);
    
    // XOR decryption with key
    std::string decrypted;
    decrypted.reserve(encrypted.size());
    
    for (size_t i = 0; i < encrypted.size(); ++i) {
        char keyChar = ENCRYPTION_KEY[i % ENCRYPTION_KEY.size()];
        decrypted.push_back(encrypted[i] ^ keyChar);
    }
    
    return decrypted;
}
