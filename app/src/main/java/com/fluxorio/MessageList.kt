package com.fluxorio

import java.util.concurrent.CopyOnWriteArrayList

/**
 * MessageList - Manages a list of messages with thread-safe operations
 */
class MessageList {
    private val messages = CopyOnWriteArrayList<Message>()
    
    /**
     * Get all messages
     */
    fun getAll(): List<Message> = messages.toList()
    
    /**
     * Get message at index
     */
    fun get(index: Int): Message = messages[index]
    
    /**
     * Add a message to the list
     */
    fun add(message: Message) {
        messages.add(message)
    }
    
    /**
     * Add a message at a specific position
     */
    fun add(index: Int, message: Message) {
        messages.add(index, message)
    }
    
    /**
     * Remove a message
     */
    fun remove(message: Message): Boolean {
        return messages.remove(message)
    }
    
    /**
     * Remove message at index
     */
    fun removeAt(index: Int): Message {
        return messages.removeAt(index)
    }
    
    /**
     * Clear all messages
     */
    fun clear() {
        messages.clear()
    }
    
    /**
     * Get the size of the message list
     */
    fun size(): Int = messages.size
    
    /**
     * Check if the list is empty
     */
    fun isEmpty(): Boolean = messages.isEmpty()
    
    /**
     * Check if the list is not empty
     */
    fun isNotEmpty(): Boolean = messages.isNotEmpty()
    
    /**
     * Get messages filtered by a predicate
     */
    fun filter(predicate: (Message) -> Boolean): List<Message> {
        return messages.filter(predicate)
    }
    
    /**
     * Get messages sent by the user
     */
    fun getSentMessages(): List<Message> {
        return messages.filter { it.isSent }
    }
    
    /**
     * Get messages received (not sent by user)
     */
    fun getReceivedMessages(): List<Message> {
        return messages.filter { !it.isSent }
    }
    
    /**
     * Get the last message
     */
    fun getLast(): Message? {
        return if (messages.isNotEmpty()) messages.last() else null
    }
    
    /**
     * Get the first message
     */
    fun getFirst(): Message? {
        return if (messages.isNotEmpty()) messages.first() else null
    }
    
    /**
     * Get messages within a time range
     */
    fun getMessagesBetween(startTime: Long, endTime: Long): List<Message> {
        return messages.filter { it.timestamp in startTime..endTime }
    }
    
    /**
     * Search messages by text
     */
    fun search(query: String, caseSensitive: Boolean = false): List<Message> {
        val searchQuery = if (caseSensitive) query else query.lowercase()
        return messages.filter { message ->
            val messageText = if (caseSensitive) message.text else message.text.lowercase()
            messageText.contains(searchQuery)
        }
    }
    
    /**
     * Get messages as a mutable list (for adapter compatibility)
     */
    fun toMutableList(): MutableList<Message> {
        return messages.toMutableList()
    }
}
