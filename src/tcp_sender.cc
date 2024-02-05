#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <iostream>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return abs_last_sent_sn_ - abs_last_ackn_sn_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return {};
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // if the window size is zero, transmit an empty message, and return
  if ( window_size_ == 0 ) {
    TCPSenderMessage empty_message = make_empty_message();
    if ( input_.reader().bytes_popped() == 0 ) {
      empty_message.SYN = true;
      sent_SYN_ = true;
    }
    transmit( empty_message );

    if ( empty_message.sequence_length() > 0 ) {
      if ( !timer_running_ ) {
        timer_running_ = true;
        timer_ = time_alive_;
      }
      outstanding_segments_.push( empty_message );
    }
    // increment the last sent sequence number
    abs_last_sent_sn_ += empty_message.sequence_length();
    return;
  }

  // while the last sent message is less than the last ackno + window size and there are bytes to send
  // but if the reader doesn't have any bytes to send, we can't send anything
  // && !( reader().bytes_buffered() > 0 )?
  while ( abs_last_sent_sn_ < ( abs_last_ackn_sn_ + window_size_ ) && reader().bytes_buffered() > 0 ) {
    TCPSenderMessage new_message;
    new_message.seqno = Wrap32::wrap( abs_last_sent_sn_, isn_ );

    // abs_last_sent_sn_ is the tail of the last sent message I guess

    // if we haven't popped any bytes yet, this is the first message!
    if ( input_.reader().bytes_popped() == 0 && !sent_SYN_ ) {
      new_message.SYN = true;
      sent_SYN_ = true;
    }

    // calculate the number of bytes to send, based on the window size and the number of bytes in the input stream
    // then make sure it's not more than the max payload size
    // const size_t bytes_to_send = min( min( reader().bytes_buffered(), window_size_ ), TCPConfig::MAX_PAYLOAD_SIZE
    // ); number of bytes to send is abs_last_sent_sn_ and the abs_last_ackn_sn_ and the window size matter here
    const size_t bytes_to_send = min( min( reader().bytes_buffered(), window_size_ - sequence_numbers_in_flight() ),
                                      TCPConfig::MAX_PAYLOAD_SIZE );

    // set the payload of the message to the substring here
    new_message.payload = input_.reader().peek().substr( 0, bytes_to_send );
    // pop the bytes from the input stream, so they're not sent again
    input_.reader().pop( bytes_to_send );

    // if the reader is finished, set the FIN flag
    if ( input_.reader().is_finished() ) {
      new_message.FIN = true;
      // ???? too many bytes in payload? if the window size is maxed out, we can't send a FIN or SYN?
    }

    // send the message
    transmit( new_message );
    if ( timer_running_ == false ) {
      timer_running_ = true;
      timer_ = time_alive_;
    }

    // add message to the queue
    outstanding_segments_.push( new_message );
    // increment the last sent sequence number
    abs_last_sent_sn_ += new_message.sequence_length();
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage empty_message;
  empty_message.seqno = Wrap32::wrap( abs_last_sent_sn_, isn_ );
  return empty_message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // TODO: if there is no ackno, do nothing?
  // if we get a message without an ackno, send a message with an empty payload
  // if ( !msg.ackno.has_value() ) {
  //   push( transmit );
  //   return;
  // }

  if ( msg.ackno.has_value() ) {
    // take the ackno and window size to it to get the right edge of the window
    abs_last_ackn_sn_ = msg.ackno.value().unwrap( isn_, reader().bytes_popped() );
    // set the window size
    window_size_ = msg.window_size;

    // remove all segments from the queue that have been acknowledged
    while ( !outstanding_segments_.empty() ) {
      if ( outstanding_segments_.front().seqno.unwrap( isn_, reader().bytes_popped() )
           < ( abs_last_ackn_sn_ + window_size_ ) ) {
        outstanding_segments_.pop();
      } else {
        break;
      }
    }
  } else {
    return; // no ackno, do nothing
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  time_alive_ += ms_since_last_tick;

  if ( time_alive_ - timer_ >= RTO_ms_ ) {
    if ( !outstanding_segments_.empty() ) {
      transmit( outstanding_segments_.front() );
      outstanding_segments_.pop();
      retransmitted_count_++;
      RTO_ms_ *= 2;
    } 
    // else {
    //   timer_running_ = false;
    //   retransmitted_count_ = 0;
    //   RTO_ms_ = initial_RTO_ms_;
    //   return;
    // }
  }
}
