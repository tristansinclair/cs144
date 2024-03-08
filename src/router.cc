#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  RouteRule rule = { route_prefix, prefix_length, next_hop, interface_num };
  _route_rules.push_back( rule );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // for each interface in the router
  //   std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};
  for ( const auto& interface : _interfaces ) {
    // queue<InternetDatagram>& queue = interface->datagrams_received();

    while ( !interface->datagrams_received().empty() ) {
      InternetDatagram curDatagram = interface->datagrams_received().front();

      if ( curDatagram.header.ttl <= 1 ) {
        interface->datagrams_received().pop();
        continue;
      }

      // decrement the TTL
      curDatagram.header.ttl--;

      // intiialize variables to keep track of the longest prefix length and the route to use
      uint8_t bestPrefixLength = 0;
      RouteRule* bestRoute = nullptr;

      for ( RouteRule& rule : _route_rules ) {
        // if the rule's prefix length is longer than the current max prefix length
        if ( rule.prefix_length >= bestPrefixLength ) {

          if ( rule.prefix_length == 0 ) {
            bestPrefixLength = rule.prefix_length;
            bestRoute = &rule;
            continue;
          }

          // if the rule's prefix matches the datagram's destination address
          uint32_t mask = 0xFFFFFFFF << ( 32 - rule.prefix_length );
          if ( ( rule.route_prefix & mask ) == ( curDatagram.header.dst & mask ) ) {
            bestPrefixLength = rule.prefix_length;
            bestRoute = &rule;
          }
        }
      }

      if ( bestRoute ) {
        // find the next hop address, if there isn't one, it's the datagram's final destination
        Address next_hop = bestRoute->next_hop.has_value() ? bestRoute->next_hop.value()
                                                           : Address::from_ipv4_numeric( curDatagram.header.dst );
        // send the datagram out on the best route's interface
        curDatagram.header.compute_checksum();
        _interfaces[bestRoute->interface_num]->send_datagram( curDatagram, next_hop );
      }
      interface->datagrams_received().pop();
    }
  }
}
