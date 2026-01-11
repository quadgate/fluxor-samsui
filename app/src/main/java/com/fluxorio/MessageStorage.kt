package com.fluxorio

import android.content.Context

/**
 * MessageStorage - Handles local persistence of messages using blob storage
 * Wrapper around BlobStorage to maintain compatibility
 */
class MessageStorage(private val context: Context) {
    
    private val blobStorage = BlobStorage(context)
    
    /**
     * Save messages to local storage
     */
    fun saveMessages(messages: List<Message>) {
        blobStorage.saveMessages(messages)
    }
    
    /**
     * Load messages from local storage
     */
    fun loadMessages(): List<Message> {
        return blobStorage.loadMessages()
    }
    
    /**
     * Clear all stored messages
     */
    fun clearMessages() {
        blobStorage.clearMessages()
    }
    
    /**
     * Check if messages exist in storage
     */
    fun hasMessages(): Boolean {
        return blobStorage.hasMessages()
    }
}
