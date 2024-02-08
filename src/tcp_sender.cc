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
  return retransmitted_count_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  cout << "input_.reader().is_finished()" << input_.reader().is_finished() << endl;

  // if the window size is zero, transmit an empty message, and return
  if ( window_size_ == 0 ) {
    TCPSenderMessage empty_message = make_empty_message();
    if ( input_.reader().bytes_popped() == 0 ) {
      empty_message.SYN = true;
      sent_SYN_ = true;
    }

    if ( input_.reader().is_finished() && !sent_FIN_ ) {
      empty_message.FIN = true;
      sent_FIN_ = true;
    }

    transmit( empty_message );

    if ( empty_message.sequence_length() > 0 ) {
      if ( !timer_running_ ) {
        timer_running_ = true;
        timer_ = 0;
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

    // if we haven't popped any bytes yet, this is the first message!
    if ( input_.reader().bytes_popped() == 0 && !sent_SYN_ ) {
      new_message.SYN = true;
      sent_SYN_ = true;
    }

    const size_t bytes_to_send = min( min( reader().bytes_buffered(), window_size_ - sequence_numbers_in_flight() ),
                                      TCPConfig::MAX_PAYLOAD_SIZE );

    // set the payload of the message to the substring here
    new_message.payload = input_.reader().peek().substr( 0, bytes_to_send );
    // pop the bytes from the input stream, so they're not sent again
    input_.reader().pop( bytes_to_send );

    // if the reader is finished, set the FIN flag
    if ( input_.reader().is_finished() && !sent_FIN_ ) {
      // Check if there's enough space in the window for both the payload and the FIN flag
      if ( bytes_to_send < ( window_size_ - sequence_numbers_in_flight() ) ) {
        new_message.FIN = true;
        sent_FIN_ = true;
      }
    }

    // send the message
    transmit( new_message );
    if ( timer_running_ == false ) {
      timer_running_ = true;
    }

    outstanding_segments_.push( new_message );
    abs_last_sent_sn_ += new_message.sequence_length();
  }

  // if the reader is finished, and there are no more bytes to send, and the queue is empty, send a FIN
  if ( input_.reader().is_finished() && window_size_ - sequence_numbers_in_flight() && !sent_FIN_ ) {
    TCPSenderMessage fin_message
      = make_empty_message(); // Assuming make_empty_message() correctly sets the sequence number
    fin_message.FIN = true;
    sent_FIN_ = true; // Mark FIN as sent
    transmit( fin_message );

    if ( timer_running_ == false ) {
      timer_running_ = true;
    }
    outstanding_segments_.push( fin_message );
    abs_last_sent_sn_ += fin_message.sequence_length();
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
      if ( outstanding_segments_.front().seqno.unwrap( isn_, reader().bytes_popped() ) < abs_last_ackn_sn_ ) {
        outstanding_segments_.pop();
        timer_running_ = false;
        timer_ = 0;
        retransmitted_count_ = 0;
        RTO_ms_ = initial_RTO_ms_;
      } else {
        break;
      }
    }
    // if the queue is empty, stop the timer
    if ( outstanding_segments_.empty() ) {
      timer_running_ = false;
      timer_ = 0;
      retransmitted_count_ = 0;
      RTO_ms_ = initial_RTO_ms_;
    }
  } else {
    return; // no ackno, do nothing
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // time_alive_ += ms_since_last_tick;
  timer_ += ms_since_last_tick;

  if ( timer_ >= RTO_ms_ ) {
    if ( !outstanding_segments_.empty() ) {
      transmit( outstanding_segments_.front() );
      // outstanding_segments_.pop();
      retransmitted_count_++;
      timer_ = 0;
      RTO_ms_ *= 2;
    } else {
      timer_running_ = false;
      timer_ = 0;
      retransmitted_count_ = 0;
      RTO_ms_ = initial_RTO_ms_;
      return;
    }
  }
}
