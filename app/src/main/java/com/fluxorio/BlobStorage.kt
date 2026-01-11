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
    }
    
    private val messagesFile: File by lazy {
        val dir = File(context.filesDir, STORAGE_DIR)
        if (!dir.exists()) {
            dir.mkdirs()
        }
        File(dir, MESSAGES_FILE)
    }
    
    /**
     * Save messages to blob storage
     */
    fun saveMessages(messages: List<Message>) {
        try {
            FileOutputStream(messagesFile).use { fos ->
                DataOutputStream(BufferedOutputStream(fos)).use { dos ->
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
                        dos.writeByte(message.messageType.ordinal.toByte())
                        
                        // Write timestamp (long as 8 bytes)
                        dos.writeLong(message.timestamp)
                    }
                    
                    dos.flush()
                }
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
        if (!messagesFile.exists() || !messagesFile.canRead()) {
            return emptyList()
        }
        
        return try {
            FileInputStream(messagesFile).use { fis ->
                DataInputStream(BufferedInputStream(fis)).use { dis ->
                    val messages = mutableListOf<Message>()
                    
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
                        val isSent = dis.readByte() == 1.toByte()
                        
                        // Read messageType (enum ordinal)
                        val messageTypeOrdinal = dis.readByte().toInt() and 0xFF
                        val messageType = MessageType.values().getOrElse(messageTypeOrdinal) { MessageType.SHORT_MESSAGE }
                        
                        // Read timestamp
                        val timestamp = dis.readLong()
                        
                        messages.add(Message(text, isSent, messageType, timestamp))
                    }
                    
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
            if (messagesFile.exists()) {
                messagesFile.delete()
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
    
    /**
     * Check if messages exist in storage
     */
    fun hasMessages(): Boolean {
        return messagesFile.exists() && messagesFile.length() > 0
    }
    
    /**
     * Get storage file size in bytes
     */
    fun getStorageSize(): Long {
        return if (messagesFile.exists()) messagesFile.length() else 0L
    }
}
