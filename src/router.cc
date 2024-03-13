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

void Router::route()
{
  for ( const auto& interface : _interfaces ) {

    // Process each datagram in the queue
    while ( !interface->datagrams_received().empty() ) {
      InternetDatagram curDatagram = interface->datagrams_received().front();

      // Drop the datagram if its TTL is 1 or less, ensuring it's not forwarded with a TTL of 0 or less
      if ( curDatagram.header.ttl <= 1 ) {
        interface->datagrams_received().pop();
        continue;
      }

      // decrement the TTL
      curDatagram.header.ttl--;

      uint8_t bestPrefixLength = 0;
      RouteRule* bestRoute = nullptr;

      // Iterate through all routing rules to find the best match based on the longest prefix match
      for ( RouteRule& rule : _route_rules ) {

        // Only look to see if the rule's prefix is better or equal to the current best
        if ( rule.prefix_length >= bestPrefixLength ) {
          if ( rule.prefix_length == 0 ) {
            bestPrefixLength = rule.prefix_length;
            bestRoute = &rule;
            continue;
          }

          uint32_t mask = 0xFFFFFFFF << ( 32 - rule.prefix_length );

          // Compare the high-order bits of the datagram's destination and the rule's prefix using the bitmask
          if ( ( rule.route_prefix & mask ) == ( curDatagram.header.dst & mask ) ) {
            bestPrefixLength = rule.prefix_length;
            bestRoute = &rule;
          }
        }
      }

      // If a matching route is found, forward the datagram accordingly
      if ( bestRoute ) {
        Address next_hop = bestRoute->next_hop.has_value() ? bestRoute->next_hop.value()
                                                           : Address::from_ipv4_numeric( curDatagram.header.dst );
        // Forward the datagram through the designated interface
        curDatagram.header.compute_checksum();
        _interfaces[bestRoute->interface_num]->send_datagram( curDatagram, next_hop );
      }

      // Remove the processed datagram from the queue
      interface->datagrams_received().pop();
    }
  }
}