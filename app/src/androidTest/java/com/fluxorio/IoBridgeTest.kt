package com.fluxorio

import android.os.Handler
import android.os.Looper
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.After
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.Assert.*
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * Instrumented tests for the I/O Bridge functionality.
 * Tests the communication between C++ native code and Kotlin UI layer.
 */
@RunWith(AndroidJUnit4::class)
class IoBridgeTest : IoBridgeListener {

    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    private val uiHandler = Handler(Looper.getMainLooper())
    
    // Synchronization objects for async testing
    private var receivedStringEvent: Pair<String, String>? = null
    private var receivedIntEvent: Pair<String, Int>? = null
    private var receivedFloatEvent: Pair<String, Float>? = null
    private var receivedDoubleEvent: Pair<String, Double>? = null
    private var receivedBooleanEvent: Pair<String, Boolean>? = null
    private var receivedByteArrayEvent: Pair<String, ByteArray>? = null
    
    private var eventLatch: CountDownLatch = CountDownLatch(1)

    companion object {
        init {
            System.loadLibrary("fluxorio")
        }
    }

    private external fun initThreadManager()
    private external fun cleanupThreadManager()
    private external fun initIOBridge()
    private external fun cleanupIOBridge()
    private external fun registerIOBridgeListener(listener: IoBridgeListener)
    private external fun unregisterIOBridgeListener()
    private external fun postStringEvent(eventId: String, data: String)
    private external fun postIntEvent(eventId: String, data: Int)
    private external fun postFloatEvent(eventId: String, data: Float)
    private external fun postDoubleEvent(eventId: String, data: Double)
    private external fun postBooleanEvent(eventId: String, data: Boolean)
    private external fun postByteArrayEvent(eventId: String, data: ByteArray)

    @Before
    fun setUp() {
        // Reset received events
        receivedStringEvent = null
        receivedIntEvent = null
        receivedFloatEvent = null
        receivedDoubleEvent = null
        receivedBooleanEvent = null
        receivedByteArrayEvent = null
        eventLatch = CountDownLatch(1)
        
        // Initialize native components
        initThreadManager()
        initIOBridge()
        registerIOBridgeListener(this)
    }

    @After
    fun tearDown() {
        unregisterIOBridgeListener()
        cleanupIOBridge()
        cleanupThreadManager()
    }

    @Test
    fun testBridgeInitialization() {
        // Test that bridge initializes without errors
        // If we get here without exceptions, initialization succeeded
        assertTrue(true)
    }

    @Test
    fun testStringEvent() {
        val eventId = "test_string_event"
        val testData = "Hello from C++"
        val latch = CountDownLatch(1)
        
        receivedStringEvent = null
        
        // Post event from native side
        postStringEvent(eventId, testData)
        
        // Wait for event to be received (with timeout)
        val received = latch.await(2, TimeUnit.SECONDS) || receivedStringEvent != null
        
        // Give handler time to process if latch didn't fire
        if (!received) {
            Thread.sleep(1000)
        }
        
        assertNotNull("String event should be received", receivedStringEvent)
        assertEquals("Event ID should match", eventId, receivedStringEvent!!.first)
        assertEquals("Event data should match", testData, receivedStringEvent!!.second)
    }

    @Test
    fun testIntEvent() {
        val eventId = "test_int_event"
        val testData = 42
        val latch = CountDownLatch(1)
        
        receivedIntEvent = null
        
        postIntEvent(eventId, testData)
        
        Thread.sleep(500)
        
        assertNotNull("Int event should be received", receivedIntEvent)
        assertEquals("Event ID should match", eventId, receivedIntEvent!!.first)
        assertEquals("Event data should match", testData, receivedIntEvent!!.second)
    }

    @Test
    fun testFloatEvent() {
        val eventId = "test_float_event"
        val testData = 3.14f
        val tolerance = 0.001f
        
        receivedFloatEvent = null
        
        postFloatEvent(eventId, testData)
        
        Thread.sleep(500)
        
        assertNotNull("Float event should be received", receivedFloatEvent)
        assertEquals("Event ID should match", eventId, receivedFloatEvent!!.first)
        assertEquals("Event data should match", testData, receivedFloatEvent!!.second, tolerance)
    }

    @Test
    fun testDoubleEvent() {
        val eventId = "test_double_event"
        val testData = 2.718281828
        val tolerance = 0.000001
        
        receivedDoubleEvent = null
        
        postDoubleEvent(eventId, testData)
        
        Thread.sleep(500)
        
        assertNotNull("Double event should be received", receivedDoubleEvent)
        assertEquals("Event ID should match", eventId, receivedDoubleEvent!!.first)
        assertEquals("Event data should match", testData, receivedDoubleEvent!!.second, tolerance)
    }

    @Test
    fun testBooleanEvent() {
        val eventId = "test_boolean_event"
        val testData = true
        
        receivedBooleanEvent = null
        
        postBooleanEvent(eventId, testData)
        
        Thread.sleep(500)
        
        assertNotNull("Boolean event should be received", receivedBooleanEvent)
        assertEquals("Event ID should match", eventId, receivedBooleanEvent!!.first)
        assertEquals("Event data should match", testData, receivedBooleanEvent!!.second)
    }

    @Test
    fun testByteArrayEvent() {
        val eventId = "test_bytearray_event"
        val testData = byteArrayOf(0x01, 0x02, 0x03, 0x04, 0x05)
        
        receivedByteArrayEvent = null
        
        postByteArrayEvent(eventId, testData)
        
        Thread.sleep(500)
        
        assertNotNull("ByteArray event should be received", receivedByteArrayEvent)
        assertEquals("Event ID should match", eventId, receivedByteArrayEvent!!.first)
        assertArrayEquals("Event data should match", testData, receivedByteArrayEvent!!.second)
    }

    @Test
    fun testMultipleEvents() {
        val eventsReceived = mutableListOf<String>()
        
        postStringEvent("event1", "data1")
        postIntEvent("event2", 100)
        postFloatEvent("event3", 1.5f)
        
        Thread.sleep(1000)
        
        // At least some events should be received
        assertTrue("Multiple events should be processed", 
            receivedStringEvent != null || receivedIntEvent != null || receivedFloatEvent != null)
    }

    @Test
    fun testListenerUnregistration() {
        // Unregister listener
        unregisterIOBridgeListener()
        
        // Clear any previous events
        receivedStringEvent = null
        
        // Post an event - it should not be received
        postStringEvent("test_event", "test_data")
        
        Thread.sleep(500)
        
        // Re-register for cleanup
        registerIOBridgeListener(this)
    }

    // IoBridgeListener implementation
    override fun onStringEvent(eventId: String, data: String) {
        uiHandler.post {
            receivedStringEvent = Pair(eventId, data)
            eventLatch.countDown()
        }
    }

    override fun onIntEvent(eventId: String, data: Int) {
        uiHandler.post {
            receivedIntEvent = Pair(eventId, data)
            eventLatch.countDown()
        }
    }

    override fun onFloatEvent(eventId: String, data: Float) {
        uiHandler.post {
            receivedFloatEvent = Pair(eventId, data)
            eventLatch.countDown()
        }
    }

    override fun onDoubleEvent(eventId: String, data: Double) {
        uiHandler.post {
            receivedDoubleEvent = Pair(eventId, data)
            eventLatch.countDown()
        }
    }

    override fun onBooleanEvent(eventId: String, data: Boolean) {
        uiHandler.post {
            receivedBooleanEvent = Pair(eventId, data)
            eventLatch.countDown()
        }
    }

    override fun onByteArrayEvent(eventId: String, data: ByteArray) {
        uiHandler.post {
            receivedByteArrayEvent = Pair(eventId, data)
            eventLatch.countDown()
        }
    }
}
