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
  size_t window = window_size_ > 0 ? window_size_ : 1;

  if ( window_size_ == 0 && sequence_numbers_in_flight() > 0 ) {
    return;
  }

  while ( abs_last_sent_sn_ < ( abs_last_ackn_sn_ + window ) ) {
    TCPSenderMessage new_message = make_empty_message();

    size_t space = min( window - sequence_numbers_in_flight(), TCPConfig::MAX_PAYLOAD_SIZE + 2 );

    if ( space > 0 && !sent_SYN_ ) {
      new_message.SYN = true;
      sent_SYN_ = true;
      space--;
    }

    if ( space > 0 ) {
      read( input_.reader(), min( space, TCPConfig::MAX_PAYLOAD_SIZE ), new_message.payload );
      space -= new_message.payload.size();
    }

    if ( reader().is_finished() && space > 0 && !sent_FIN_ ) {
      new_message.FIN = true;
      sent_FIN_ = true;
      space--;
    }

    if ( new_message.sequence_length() == 0 ) {
      break;
    }

    transmit( new_message );
    outstanding_segments_.push( new_message );
    abs_last_sent_sn_ += new_message.sequence_length();

    if ( timer_running_ == false ) {
      timer_running_ = true;
      timer_ = 0;
    }
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
  if ( msg.ackno.has_value() ) {

    // set the window size
    window_size_ = msg.window_size;

    // handle case is ackno received is beyond the last sent sequence number
    if ( msg.ackno.value().unwrap( isn_, reader().bytes_popped() ) > abs_last_sent_sn_ ) {
      return;
    }

    if ( msg.ackno.value().unwrap( isn_, reader().bytes_popped() ) <= abs_last_ackn_sn_ ) {
      return;
    }

    // take the ackno and window size to it to get the right edge of the window
    abs_last_ackn_sn_ = msg.ackno.value().unwrap( isn_, reader().bytes_popped() );

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
    window_size_ = msg.window_size;
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
      if ( window_size_ != 0 ) {
        RTO_ms_ *= 2;
      }
    } else {
      timer_running_ = false;
    }
  }
}
