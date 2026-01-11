package com.fluxorio

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.card.MaterialCardView

class MessageAdapter(private val messageList: MessageList) :
    RecyclerView.Adapter<MessageAdapter.MessageViewHolder>() {

    class MessageViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val messageText: TextView = itemView.findViewById(R.id.messageText)
        val messageCard: MaterialCardView = itemView.findViewById(R.id.messageCard)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): MessageViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_message, parent, false)
        return MessageViewHolder(view)
    }

    override fun onBindViewHolder(holder: MessageViewHolder, position: Int) {
        val message = messageList.get(position)
        holder.messageText.text = message.text
        
        val params = holder.messageCard.layoutParams as androidx.constraintlayout.widget.ConstraintLayout.LayoutParams
        
        // Check if message is from internet (socket message)
        val isInternetMessage = message.text.startsWith("[Socket]")
        
        // Style based on whether message is sent, received, or from internet
        if (message.isSent) {
            holder.messageCard.setCardBackgroundColor(
                ContextCompat.getColor(holder.itemView.context, R.color.message_sent)
            )
            params.horizontalBias = 1.0f
        } else if (isInternetMessage) {
            // Center messages from internet
            holder.messageCard.setCardBackgroundColor(
                ContextCompat.getColor(holder.itemView.context, R.color.message_received)
            )
            params.horizontalBias = 0.5f
        } else {
            holder.messageCard.setCardBackgroundColor(
                ContextCompat.getColor(holder.itemView.context, R.color.message_received)
            )
            params.horizontalBias = 0.0f
        }
        holder.messageCard.layoutParams = params
    }

    override fun getItemCount() = messageList.size()

    fun addMessage(message: Message) {
        messageList.add(message)
        notifyItemInserted(messageList.size() - 1)
    }
    
    fun clearMessages() {
        val size = messageList.size()
        messageList.clear()
        notifyItemRangeRemoved(0, size)
    }
    
    fun removeMessage(position: Int) {
        if (position in 0 until messageList.size()) {
            messageList.removeAt(position)
            notifyItemRemoved(position)
        }
    }
}

