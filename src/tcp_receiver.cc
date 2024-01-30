#include "tcp_receiver.hh"
#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reader().set_error(); // set the error flag
    return;
  }

  // no zero_point and no SYN = abort
  if ( !zero_point_ && !message.SYN ) {
    return;
  }

  if ( message.SYN ) {
    zero_point_ = message.seqno;                            // set the initial sequence number (zero point)
    reassembler_.insert( 0, message.payload, message.FIN ); // insert @ 0
  }

  // convert seqno to index for resassembler
  uint64_t streamIndex = message.seqno.unwrap( zero_point_.value(), writer().bytes_pushed() ) - 1;
  reassembler_.insert( streamIndex, message.payload, message.FIN );
};

TCPReceiverMessage TCPReceiver::send() const
{
  if ( reassembler_.reader().has_error() ) {
    return { nullopt, 0, true }; // RST case
  }

  TCPReceiverMessage message;
  if ( zero_point_ ) {
    message.ackno = Wrap32::wrap( writer().bytes_pushed() + 1, zero_point_.value() );

    if ( writer().is_closed() ) {
      message.ackno = message.ackno.value() + 1; // add the FIN byte
    }
  }

  message.window_size = writer().available_capacity() >= UINT16_MAX
                          ? UINT16_MAX
                          : writer().available_capacity(); // min of UINT16_MAX and the available space

  return message;
}
