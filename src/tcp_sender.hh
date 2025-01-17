#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  uint64_t RTO_ms_ { initial_RTO_ms_ };
  uint64_t time_alive_ { 0 };

  bool timer_running_ { false };
  uint64_t timer_ { 0 };
  std::queue<TCPSenderMessage> outstanding_segments_ {};
  uint16_t retransmitted_count_ { 0 };
  // Wrap32 last_seen_sq_ {}; make this initialized to isn_ in constructor

  // vars used for holding info regarding the window size and how much to send
  // uint64_t last_ackn_sn_ { 0 };
  size_t window_size_ { 1 };
  // uint64_t last_sent_sn_ { 0 };
  uint64_t abs_last_ackn_sn_ { 0 };
  uint64_t abs_last_sent_sn_ { 0 };

  bool sent_SYN_ { false };
  bool sent_FIN_ { false };
  uint64_t bytes_sent { 0 };
};
