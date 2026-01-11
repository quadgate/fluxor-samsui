package com.fluxorio

/**
 * Interface for receiving events from the C++ I/O Bridge.
 * All callback methods are called from background threads and should
 * post updates to the UI thread if needed.
 */
interface IoBridgeListener {
    /**
     * Called when a string event is received from C++
     * @param eventId Unique identifier for the event
     * @param data String data
     */
    fun onStringEvent(eventId: String, data: String)
    
    /**
     * Called when an integer event is received from C++
     * @param eventId Unique identifier for the event
     * @param data Integer data
     */
    fun onIntEvent(eventId: String, data: Int)
    
    /**
     * Called when a float event is received from C++
     * @param eventId Unique identifier for the event
     * @param data Float data
     */
    fun onFloatEvent(eventId: String, data: Float)
    
    /**
     * Called when a double event is received from C++
     * @param eventId Unique identifier for the event
     * @param data Double data
     */
    fun onDoubleEvent(eventId: String, data: Double)
    
    /**
     * Called when a boolean event is received from C++
     * @param eventId Unique identifier for the event
     * @param data Boolean data
     */
    fun onBooleanEvent(eventId: String, data: Boolean)
    
    /**
     * Called when a byte array event is received from C++
     * @param eventId Unique identifier for the event
     * @param data Byte array data
     */
    fun onByteArrayEvent(eventId: String, data: ByteArray)
}
