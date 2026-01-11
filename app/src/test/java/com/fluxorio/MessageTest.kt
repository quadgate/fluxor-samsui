package com.fluxorio

import org.junit.Test
import org.junit.Assert.*

/**
 * Unit tests for the Message data class.
 */
class MessageTest {

    @Test
    fun testMessageCreation() {
        val message = Message(
            text = "Test message",
            isSent = true
        )
        
        assertEquals("Test message", message.text)
        assertTrue(message.isSent)
        assertTrue(message.timestamp > 0)
    }

    @Test
    fun testMessageEquality() {
        val timestamp = System.currentTimeMillis()
        val message1 = Message("Hello", true, timestamp)
        val message2 = Message("Hello", true, timestamp)
        
        assertEquals(message1, message2)
        assertEquals(message1.hashCode(), message2.hashCode())
    }

    @Test
    fun testMessageInequality() {
        val message1 = Message("Hello", true)
        val message2 = Message("World", true)
        
        assertNotEquals(message1, message2)
    }

    @Test
    fun testMessageSentFlag() {
        val sentMessage = Message("Sent", true)
        val receivedMessage = Message("Received", false)
        
        assertTrue(sentMessage.isSent)
        assertFalse(receivedMessage.isSent)
    }

    @Test
    fun testMessageDefaultTimestamp() {
        val before = System.currentTimeMillis()
        val message = Message("Test", false)
        val after = System.currentTimeMillis()
        
        assertTrue(message.timestamp >= before)
        assertTrue(message.timestamp <= after)
    }
}
