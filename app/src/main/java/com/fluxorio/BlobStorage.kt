package com.fluxorio

import android.content.Context
import java.io.*

/**
 * BlobStorage - Handles local persistence of messages using binary file storage
 */
class BlobStorage(private val context: Context) {
    
    companion object {
        private const val STORAGE_DIR = "messages"
        private const val MESSAGES_FILE = "messages.blob"
        
        init {
            System.loadLibrary("fluxorio")
        }
        
        // Native method declarations
        private external fun saveMessagesNative(filePath: String, data: ByteArray): Boolean
        private external fun loadMessagesNative(filePath: String): ByteArray?
        private external fun clearMessagesNative(filePath: String): Boolean
        private external fun hasMessagesNative(filePath: String): Boolean
        private external fun getStorageSizeNative(filePath: String): Long
    }
    
    private val messagesFile: File by lazy {
        val dir = File(context.filesDir, STORAGE_DIR)
        File(dir, MESSAGES_FILE)
    }
    
    private val messagesFilePath: String
        get() = messagesFile.absolutePath
    
    /**
     * Serialize messages to byte array using the same binary format
     */
    private fun serializeMessages(messages: List<Message>): ByteArray {
        val baos = ByteArrayOutputStream()
        DataOutputStream(baos).use { dos ->
            // Write message count
            dos.writeInt(messages.size)
            
            // Write each message
            messages.forEach { message ->
                // Write text length and text bytes
                val textBytes = message.text.toByteArray(Charsets.UTF_8)
                dos.writeInt(textBytes.size)
                dos.write(textBytes)
                
                // Write isSent (boolean as 1 byte)
                dos.writeByte(if (message.isSent) 1 else 0)
                
                // Write messageType (enum ordinal as 1 byte)
                dos.writeByte(message.messageType.ordinal)
                
                // Write timestamp (long as 8 bytes)
                dos.writeLong(message.timestamp)
            }
        }
        return baos.toByteArray()
    }
    
    /**
     * Deserialize messages from byte array using the same binary format
     */
    private fun deserializeMessages(data: ByteArray): List<Message> {
        if (data.isEmpty()) {
            return emptyList()
        }
        
        return try {
            val messages = mutableListOf<Message>()
            DataInputStream(ByteArrayInputStream(data)).use { dis ->
                // Read message count
                val count = dis.readInt()
                
                // Read each message
                for (i in 0 until count) {
                    // Read text length and text bytes
                    val textLength = dis.readInt()
                    val textBytes = ByteArray(textLength)
                    dis.readFully(textBytes)
                    val text = String(textBytes, Charsets.UTF_8)
                    
                    // Read isSent
                    val isSent = dis.readByte().toInt() == 1
                    
                    // Read messageType (enum ordinal)
                    val messageTypeOrdinal = dis.readByte().toInt() and 0xFF
                    val messageType = MessageType.values().getOrElse(messageTypeOrdinal) { MessageType.SHORT_MESSAGE }
                    
                    // Read timestamp
                    val timestamp = dis.readLong()
                    
                    messages.add(Message(text, isSent, messageType, timestamp))
                }
            }
            messages
        } catch (e: Exception) {
            e.printStackTrace()
            emptyList()
        }
    }
    
    /**
     * Save messages to blob storage
     */
    fun saveMessages(messages: List<Message>) {
        try {
            val data = serializeMessages(messages)
            if (!saveMessagesNative(messagesFilePath, data)) {
                // Log error (in production, use proper logging)
                System.err.println("Failed to save messages to native storage")
            }
        } catch (e: Exception) {
            // Log error (in production, use proper logging)
            e.printStackTrace()
        }
    }
    
    /**
     * Load messages from blob storage
     */
    fun loadMessages(): List<Message> {
        return try {
            val data = loadMessagesNative(messagesFilePath)
            if (data == null || data.isEmpty()) {
                emptyList()
            } else {
                val messages = deserializeMessages(data)
                if (messages.isEmpty() && data.isNotEmpty()) {
                    // Deserialization failed, clear corrupted file
                    clearMessages()
                    emptyList()
                } else {
                    messages
                }
            }
        } catch (e: Exception) {
            // If reading fails, return empty list and clear corrupted file
            clearMessages()
            emptyList()
        }
    }
    
    /**
     * Clear all stored messages
     */
    fun clearMessages() {
        try {
            clearMessagesNative(messagesFilePath)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
    
    /**
     * Check if messages exist in storage
     */
    fun hasMessages(): Boolean {
        return try {
            hasMessagesNative(messagesFilePath)
        } catch (e: Exception) {
            false
        }
    }
    
    /**
     * Get storage file size in bytes
     */
    fun getStorageSize(): Long {
        return try {
            getStorageSizeNative(messagesFilePath)
        } catch (e: Exception) {
            0L
        }
    }
}
