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
  //   The TCPSender is asked to fill the window from the outbound byte stream: it reads from the stream and sends
  //   as many TCPSenderMessages as possible, as long as there are new bytes to be read and space available in the
  //   window. It sends them by calling the provided transmit() function on them.
  // You’ll want to make sure that every TCPSenderMessage you send fits fully inside the receiver’s window. Make
  // each individual message as big as possible, but no bigger than the value given by TCPConfig::MAX PAYLOAD SIZE
  // (1452 bytes). You can use the TCPSenderMessage::sequence length() method to count the total number of sequence
  // numbers occupied by a segment. Remember that the SYN and FIN flags also occupy a sequence number each, which
  // means that they occupy space in the window.

  // ⋆What should I do if the window size is zero? If the receiver has announced a window size of zero, the push
  // method should pretend like the window size is one. The sender might end up sending a single byte that gets
  // rejected (and not acknowledged) by the receiver, but this can also provoke the receiver into sending a new
  // acknowledgment segment where it reveals that more space has opened up in its window. Without this, the sender
  // would never learn that it was allowed to start sending again. This is the only special-case behavior your
  // implementation should have for the case of a zero-size window. The TCPSender shouldn’t actually remember a
  // false window size of 1. The special case is only within the push method. Also, N.B. that even if the window
  // size is one (or 20, or 200), the window might still be full. A “full” window is not the same as a “zero-size”
  // window.

  // look at the window size and the last seen sequence number and send as many segments as possible

  // check if there is space  between the last seen sequence number and the last ackno given the window size
  // if there is, send segments until there is no space left or no more bytes to send from the input stream

  cout << "TCPSender::push " << endl;
  cout << "abs_last_sent_sn_ " << abs_last_sent_sn_ << endl;
  cout << "abs_last_ackn_sn_ " << abs_last_ackn_sn_ << endl;
  cout << "window_size_ " << window_size_ << endl;
  cout << "isn_ " << isn_.unwrap( Wrap32( 0 ), 0 ) << endl;

  // if the window size is zero, transmit an empty message, and return
  if ( window_size_ == 0 ) {
    TCPSenderMessage empty_message = make_empty_message();
    if ( input_.reader().bytes_popped() == 0 ) {
      empty_message.SYN = true;
      sent_SYN_ = true;
    }
    transmit( empty_message );
    // increment the last sent sequence number
    abs_last_sent_sn_ += empty_message.sequence_length();
    return;
  }

  // while the last sent message is less than the last ackno + window size and there are bytes to send
  // but if the reader doesn't have any bytes to send, we can't send anything
  // && !( reader().bytes_buffered() > 0 )?
  while ( abs_last_sent_sn_ < abs_last_ackn_sn_ && !( reader().bytes_buffered() > 0 ) ) {
    // cout << "Creating new message" << endl;
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
    const size_t bytes_to_send = min( min( reader().bytes_buffered(), window_size_ ), TCPConfig::MAX_PAYLOAD_SIZE );
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
    // add message to the queue
    outstanding_segments_.push( new_message );
    // increment the last sent sequence number
    abs_last_sent_sn_ += new_message.sequence_length();
  }

  // if there are no more bytes to send, return
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // make an empty message
  TCPSenderMessage empty_message;
  empty_message.seqno = Wrap32::wrap( abs_last_sent_sn_, isn_ );
  // if ( input_.reader().bytes_popped() == 0 ) {
  //   empty_message.SYN = true;
  // sent_SYN_ = true;
  // }

  // if ( input_.reader().is_finished() ) {
  //   empty_message.FIN = true;
  // }
  // cout << "transmitting message. seq length: " << empty_message.sequence_length() << endl;

  return empty_message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // A message is received from the receiver, conveying the new left (= ackno) and right (= ackno + window size)
  // edges of the window. The TCPSender should look through its collection of outstanding segments and remove any
  // that have now been fully acknowl- edged (the ackno is greater than all of the sequence numbers in the segment).
  // struct TCPReceiverMessage
  // {
  //   std::optional<Wrap32> ackno {};
  //   uint16_t window_size {};
  //   bool RST {};
  // };

  // handle the rst case TODO: is this correct? maybe just close the connection
  // if ( msg.RST ) {
  //   writer().set_error();
  //   return;
  // }

  // TODO: if there is no ackno, do nothing?
  // if we get a message without an ackno, send a message with an empty payload
  // if ( !msg.ackno.has_value() ) {
  //   push( transmit );
  //   return;
  // }

  cout << "TCPSender::receive " << endl;

  if ( msg.ackno.has_value() ) {
    cout << "msg.ackno " << msg.ackno.value().unwrap( Wrap32( 0 ), 0 ) << endl;
    cout << "msg.window_size " << msg.window_size << endl;
    cout << "msg.RST " << msg.RST << endl;
    // take the ackno and window size to it to get the right edge of the window
    uint64_t abs_sn_ = msg.ackno.value().unwrap( isn_, reader().bytes_popped() ) + msg.window_size;
    abs_last_ackn_sn_ = abs_sn_;

    // set the window size
    window_size_ = msg.window_size;

    // remove all segments from the queue that have been acknowledged
    while ( !outstanding_segments_.empty() ) {
      if ( outstanding_segments_.front().seqno.unwrap( isn_, reader().bytes_popped() ) < abs_sn_ ) {
        outstanding_segments_.pop();
      } else {
        break;
      }
    }
  } else {
    cout << "no ackno" << endl;
    return; // no ackno, do nothing
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;

  // Time has passed — a certain number of milliseconds since the last time this method was called. The sender may
  // need to retransmit an outstanding segment; it can call the transmit() function to do this. (Reminder: please
  // don’t try to use real-world “clock” or “gettimeofday” functions in your code; the only reference to time
  // passing comes from the ms since last tick argument.)

  // check if there are any outstanding segments
  // if there are, check if the oldest segment has been outstanding for longer than the current retransmission
  // timeout if it has, retransmit it if it hasn't, do nothing if there are no outstanding segments, do nothing
  // outstanding_segments_

  // if ms_since_last_tick

  if ( outstanding_segments_.empty() ) {
    return;
  }
}
