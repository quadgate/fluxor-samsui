package com.fluxorio

import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.inputmethod.EditorInfo
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import com.fluxorio.databinding.ActivityMainBinding
import java.io.InputStream

class MainActivity : AppCompatActivity(), IoBridgeListener {

    companion object {
        init {
            System.loadLibrary("fluxorio")
        }
    }

    private lateinit var binding: ActivityMainBinding
    private lateinit var messageAdapter: MessageAdapter
    private val messageList = MessageList()
    private val uiHandler = Handler(Looper.getMainLooper())
    
    // Image picker launcher
    private val imagePickerLauncher = registerForActivityResult(ActivityResultContracts.GetContent()) { uri: Uri? ->
        uri?.let { handleImageSelection(it) }
    }

    // Native method declarations
    private external fun initThreadManager()
    private external fun cleanupThreadManager()
    private external fun initIOBridge()
    private external fun cleanupIOBridge()
    private external fun registerIOBridgeListener(listener: IoBridgeListener)
    private external fun unregisterIOBridgeListener()
    private external fun sendMessageToThreadHandler(message: String)
    private external fun sendImageToThreadHandler(imageData: ByteArray)
    
    // Socket manager native methods
    private external fun initSocketManager()
    private external fun cleanupSocketManager()
    private external fun startSocketServer(port: Int): Boolean
    private external fun stopSocketServer()
    private external fun sendMessageToClients(message: String)
    private external fun getConnectedClientCount(): Int

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupRecyclerView()
        setupInputField()
        
        // Initialize native components
        initThreadManager()
        initIOBridge()
        registerIOBridgeListener(this)
        
        // Initialize and start socket manager
        initSocketManager()
        // Start TCP server on port 8888
        if (startSocketServer(8888)) {
            messageAdapter.addMessage(Message("TCP Server started on port 8888", false))
        } else {
            messageAdapter.addMessage(Message("Failed to start TCP Server", false))
        }
        
        // Add welcome message
        messageAdapter.addMessage(Message("Hello! How can I help you today?", false))
    }

    override fun onDestroy() {
        super.onDestroy()
        stopSocketServer()
        cleanupSocketManager()
        unregisterIOBridgeListener()
        cleanupIOBridge()
        cleanupThreadManager()
    }

    private fun setupRecyclerView() {
        messageAdapter = MessageAdapter(messageList)
        binding.recyclerViewMessages.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = messageAdapter
        }
    }

    private fun setupInputField() {
        binding.editTextMessage.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_SEND) {
                sendMessage()
                true
            } else {
                false
            }
        }

        binding.buttonSend.setOnClickListener {
            sendMessage()
        }
        
        binding.buttonImage.setOnClickListener {
            imagePickerLauncher.launch("image/*")
        }
    }

    private fun sendMessage() {
        val messageText = binding.editTextMessage.text?.toString()?.trim()
        if (!messageText.isNullOrEmpty()) {
            // Add user message to UI
            messageAdapter.addMessage(Message(messageText, true))
            binding.editTextMessage.text?.clear()
            
            // Scroll to bottom
            binding.recyclerViewMessages.post {
                binding.recyclerViewMessages.smoothScrollToPosition(messageAdapter.itemCount - 1)
            }
            
            // Show loading indicator
            showLoader()
            
            // Send message to C++ thread handler for processing
            // The response will come back via onStringEvent callback with eventId "message_response"
            sendMessageToThreadHandler(messageText)
            
            // Also send message to connected socket clients
            sendMessageToClients(messageText)
        }
    }
    
    private fun showLoader() {
        binding.progressIndicator.visibility = View.VISIBLE
    }
    
    private fun hideLoader() {
        binding.progressIndicator.visibility = View.GONE
    }
    
    private fun handleImageSelection(uri: Uri) {
        try {
            val inputStream: InputStream? = contentResolver.openInputStream(uri)
            val imageBytes = inputStream?.readBytes()
            inputStream?.close()
            
            if (imageBytes != null) {
                // Add user message to UI
                messageAdapter.addMessage(Message("📷 Image selected (${imageBytes.size / 1024} KB)", true))
                
                // Scroll to bottom
                binding.recyclerViewMessages.post {
                    binding.recyclerViewMessages.smoothScrollToPosition(messageAdapter.itemCount - 1)
                }
                
                // Show loading indicator
                showLoader()
                
                // Send image to C++ thread handler for processing
                sendImageToThreadHandler(imageBytes)
            }
        } catch (e: Exception) {
            messageAdapter.addMessage(Message("Error loading image: ${e.message}", false))
        }
    }

    // IoBridgeListener implementation
    override fun onStringEvent(eventId: String, data: String) {
        uiHandler.post {
            // Handle message response from thread handler
            when (eventId) {
                "message_response" -> {
                    hideLoader()
                    messageAdapter.addMessage(Message(data, false))
                }
                "socket_message" -> {
                    // Message received from socket client
                    messageAdapter.addMessage(Message("[Socket] $data", false))
                }
                "image_info" -> {
                    // Image processing info
                    messageAdapter.addMessage(Message(data, false))
                }
                else -> {
                    messageAdapter.addMessage(Message("[$eventId] $data", false))
                }
            }
            binding.recyclerViewMessages.smoothScrollToPosition(messageAdapter.itemCount - 1)
        }
    }

    override fun onIntEvent(eventId: String, data: Int) {
        uiHandler.post {
            when (eventId) {
                "socket_client_count" -> {
                    // Update UI with connected client count
                    messageAdapter.addMessage(Message("[Socket] Connected clients: $data", false))
                }
                else -> {
                    messageAdapter.addMessage(Message("[$eventId] Int: $data", false))
                }
            }
            binding.recyclerViewMessages.smoothScrollToPosition(messageAdapter.itemCount - 1)
        }
    }

    override fun onFloatEvent(eventId: String, data: Float) {
        uiHandler.post {
            messageAdapter.addMessage(Message("[$eventId] Float: $data", false))
            binding.recyclerViewMessages.smoothScrollToPosition(messageAdapter.itemCount - 1)
        }
    }

    override fun onDoubleEvent(eventId: String, data: Double) {
        uiHandler.post {
            messageAdapter.addMessage(Message("[$eventId] Double: $data", false))
            binding.recyclerViewMessages.smoothScrollToPosition(messageAdapter.itemCount - 1)
        }
    }

    override fun onBooleanEvent(eventId: String, data: Boolean) {
        uiHandler.post {
            messageAdapter.addMessage(Message("[$eventId] Boolean: $data", false))
            binding.recyclerViewMessages.smoothScrollToPosition(messageAdapter.itemCount - 1)
        }
    }

    override fun onByteArrayEvent(eventId: String, data: ByteArray) {
        uiHandler.post {
            when (eventId) {
                "image_processed" -> {
                    hideLoader()
                    messageAdapter.addMessage(Message("✅ Image processed: ${data.size} bytes returned", false))
                }
                "image_response" -> {
                    hideLoader()
                    messageAdapter.addMessage(Message("✅ Image processing complete: ${data.size} bytes", false))
                }
                else -> {
                    messageAdapter.addMessage(Message("[$eventId] ByteArray: ${data.size} bytes", false))
                }
            }
            binding.recyclerViewMessages.smoothScrollToPosition(messageAdapter.itemCount - 1)
        }
    }
}
