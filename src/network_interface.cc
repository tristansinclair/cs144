#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t next_hop_ip = next_hop.ipv4_numeric();
  if ( arp_table_.count( next_hop_ip ) ) {
    // If MAC address known, send the datagram directly.
    EthernetFrame newFrame;
    newFrame.header.src = ethernet_address_;
    newFrame.header.dst = arp_table_[next_hop_ip];
    newFrame.header.type = EthernetHeader::TYPE_IPv4;
    newFrame.payload = serialize( dgram );
    transmit( newFrame );
  } else {
    waiting_for_arp_queue_[next_hop_ip].push( dgram );
    // If MAC address unknown, check for recent ARP request, else try and send ARP request.
    if ( !arp_timer_table_.count( next_hop_ip ) || arp_timer_table_[next_hop_ip] > 5000 ) {
      ARPMessage arpRequest;
      arpRequest.opcode = ARPMessage::OPCODE_REQUEST;
      arpRequest.sender_ethernet_address = ethernet_address_;
      arpRequest.sender_ip_address = ip_address_.ipv4_numeric();
      arpRequest.target_ip_address = next_hop_ip;

      EthernetFrame newFrame;
      newFrame.header.src = ethernet_address_;
      newFrame.header.dst = ETHERNET_BROADCAST;
      newFrame.header.type = EthernetHeader::TYPE_ARP;
      newFrame.payload = serialize( arpRequest );
      // waiting_for_arp_queue_[next_hop_ip].push( dgram );
      transmit( newFrame );
      arp_timer_table_[next_hop_ip] = 0;
    }
    // Queue the datagram until ARP reply is received.
    // waiting_for_arp_queue_[next_hop_ip].push( dgram );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Ignore frames not for this interface.
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST )
    return;

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    IPv4Datagram datagram;
    if ( parse( datagram, frame.payload ) )
      datagrams_received_.push( datagram );
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arpMessage;
    if ( parse( arpMessage, frame.payload ) ) {
      if ( arpMessage.opcode == ARPMessage::OPCODE_REQUEST
           && arpMessage.target_ip_address == ip_address_.ipv4_numeric() ) {
        // Send ARP reply if this interface is the target.
        ARPMessage arpReply;
        arpReply.opcode = ARPMessage::OPCODE_REPLY;
        arpReply.sender_ethernet_address = ethernet_address_;
        arpReply.sender_ip_address = ip_address_.ipv4_numeric();
        arpReply.target_ethernet_address = arpMessage.sender_ethernet_address;
        arpReply.target_ip_address = arpMessage.sender_ip_address;

        EthernetFrame newFrame;
        newFrame.header.src = ethernet_address_;
        newFrame.header.dst = arpMessage.sender_ethernet_address;
        newFrame.header.type = EthernetHeader::TYPE_ARP;
        newFrame.payload = serialize( arpReply );
        transmit( newFrame );
      }

      // Update ARP table and process queued datagrams.
      arp_table_[arpMessage.sender_ip_address] = arpMessage.sender_ethernet_address;
      arp_exp_timer_table_[arpMessage.sender_ip_address] = 0;

      if ( waiting_for_arp_queue_.count( arpMessage.sender_ip_address ) ) {
        while ( !waiting_for_arp_queue_[arpMessage.sender_ip_address].empty() ) {
          send_datagram( waiting_for_arp_queue_[arpMessage.sender_ip_address].front(),
                         Address::from_ipv4_numeric( arpMessage.sender_ip_address ) );
          waiting_for_arp_queue_[arpMessage.sender_ip_address].pop();
        }
        waiting_for_arp_queue_.erase( arpMessage.sender_ip_address );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Remove entries from arp_table_ if their corresponding entries in 
  //the arp_exp_timer_table_ are greater than 30000
  for ( auto it = arp_exp_timer_table_.begin(); it != arp_exp_timer_table_.end(); ) {
    if ( it->second + ms_since_last_tick > 30000 ) {
      arp_table_.erase( it->first );
      it = arp_exp_timer_table_.erase( it );
    } else {
      it->second += ms_since_last_tick;
      ++it;
    }
  }

  // Remove all entries from arp_timer_table_ that are greater than 5000
  for ( auto it = arp_timer_table_.begin(); it != arp_timer_table_.end(); ) {
    it->second += ms_since_last_tick;
    if ( it->second > 5000 ) {
      it = arp_timer_table_.erase( it );
    } else {
      ++it;
    }
  }
}