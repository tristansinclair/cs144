#include "tcp_receiver.hh"
#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{

  if ( message.RST ) {
    reader().set_error();
    return;
  }

  if ( !zero_point_ && !message.SYN ) {
    return;
  }

  if ( message.SYN ) {
    zero_point_ = message.seqno; // set the initial sequence number (zero point)
    reassembler_.insert( 0, message.payload, message.FIN );
  }
  uint64_t streamIndex = message.seqno.unwrap( zero_point_.value(), writer().bytes_pushed() ) - 1;
  reassembler_.insert( streamIndex, message.payload, message.FIN );
};

TCPReceiverMessage TCPReceiver::send() const
{
  if ( reassembler_.reader().has_error() ) {
    return { nullopt, 0, true };
  }

  TCPReceiverMessage message;

  // ack is the next byte needed!
  // uint64_t firstUnassembledIndex = reader().bytes_popped() + reader().bytes_buffered();
  if ( zero_point_ ) {
    // message.ackno = Wrap32( 0 ).wrap( firstUnassembledIndex, zero_point_.value() );
    message.ackno = Wrap32::wrap( writer().bytes_pushed() + 1, zero_point_.value() );

    if ( writer().is_closed() ) {
      message.ackno = message.ackno.value() + 1;
    }

    // cout << "zero_point_.value() = " << zero_point_.value() << endl;
    // cout << "writer().bytes_pushed() = " << writer().bytes_pushed() << endl;
    // cout << "message.ackno = " << message.ackno.value() << endl;
    // cout << "writer().is_closed() = " << writer().is_closed() << endl;
  }

  // Wrap32 ack = wrap( firstUnassembledIndex, zero_point_.value() );
  // message.ackno = Wrap32( firstUnassembledIndex ).unwrap( zero_point_.value(), absolueSeqno_ );

  // take the min of UINT16_MAX and the available space
  message.window_size = writer().available_capacity() >= UINT16_MAX ? UINT16_MAX : writer().available_capacity();

  return message;
}
