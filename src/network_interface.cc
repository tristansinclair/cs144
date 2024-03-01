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
  // see if there is a MAC address for the next_hop_ip in the ARP table
  if ( arp_table_.count( next_hop_ip ) ) {

    EthernetFrame newFrame;
    newFrame.header.src = ethernet_address_;
    newFrame.header.dst = arp_table_[next_hop_ip]; // dst address from ARP table
    newFrame.header.type = EthernetHeader::TYPE_IPv4;
    newFrame.payload = serialize( dgram );
    // ethernet_frame_output_.push( newFrame );
    transmit( newFrame );

  } else {

    // if not send an ARP request for the next_hop's Ethernet address
    // check if there has been an ARP request for the next_hop's Ethernet address in the last 5 seconds
    // if so, wait for a reply to the first one
    if ( !waiting_for_arp_queue_.count( next_hop_ip ) ) { // if we're waiting already
      // send the ARP request
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
      // waiting_for_arp_.insert( { next_hop_ip, dgram } );

      transmit( newFrame );
    }
    // queue the IP datagram so it can be sent after the ARP reply is received
    if ( waiting_for_arp_queue_.count( next_hop_ip ) ) {
      waiting_for_arp_queue_[next_hop_ip].push( dgram );
    } else {
      // waiting_for_arp_queue_.insert( { next_hop_ip, { dgram } } );
      // build a queue of datagrams to send
      queue<InternetDatagram> q;
      q.push( dgram );
      waiting_for_arp_queue_.insert( { next_hop_ip, q } );
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // This method is called when an Ethernet frame arrives from the network. The code should ignore any frames not
  // destined for the network interface (meaning, the Ethernet destination is either the broadcast address or the
  // interface’s own Ethernet address stored in the ethernet address member variable).

  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST )
    return;

  // check if the inbounc frame is ipv4
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    IPv4Datagram datagram;
    if ( parse( datagram, frame.payload ) )
      datagrams_received_.push( datagram );
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arpMessage;
    if ( parse( arpMessage, frame.payload ) ) {
      if ( arpMessage.opcode == ARPMessage::OPCODE_REQUEST
           && arpMessage.target_ip_address == ip_address_.ipv4_numeric() ) {
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
      arp_table_.insert( { arpMessage.sender_ip_address, arpMessage.sender_ethernet_address } );
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
  (void)ms_since_last_tick;
}
