package com.fluxorio

enum class MessageType {
    SHORT_MESSAGE,
    LONG_MESSAGE,
    IMAGE,
    VIDEO
}

data class Message(
    val text: String,
    val isSent: Boolean,
    val messageType: MessageType = MessageType.SHORT_MESSAGE,
    val timestamp: Long = System.currentTimeMillis()
)

