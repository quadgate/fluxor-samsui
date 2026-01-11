#ifndef MESSAGE_ENCRYPTION_H
#define MESSAGE_ENCRYPTION_H

#include <string>

/**
 * Encrypt a message using XOR cipher with base64 encoding
 * @param message The message to encrypt
 * @return Encrypted message in base64 format
 */
std::string encryptMessage(const std::string& message);

/**
 * Decrypt a message (for future use if needed)
 * @param encryptedMessage The encrypted message in base64 format
 * @return Decrypted message
 */
std::string decryptMessage(const std::string& encryptedMessage);

#endif // MESSAGE_ENCRYPTION_H
