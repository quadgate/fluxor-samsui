package com.fluxorio

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.card.MaterialCardView

class MessageAdapter(private val messages: MutableList<Message>) :
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
        val message = messages[position]
        holder.messageText.text = message.text
        
        val params = holder.messageCard.layoutParams as androidx.constraintlayout.widget.ConstraintLayout.LayoutParams
        
        // Style based on whether message is sent or received
        if (message.isSent) {
            holder.messageCard.setCardBackgroundColor(
                ContextCompat.getColor(holder.itemView.context, R.color.message_sent)
            )
            params.horizontalBias = 1.0f
        } else {
            holder.messageCard.setCardBackgroundColor(
                ContextCompat.getColor(holder.itemView.context, R.color.message_received)
            )
            params.horizontalBias = 0.0f
        }
        holder.messageCard.layoutParams = params
    }

    override fun getItemCount() = messages.size

    fun addMessage(message: Message) {
        messages.add(message)
        notifyItemInserted(messages.size - 1)
    }
}

