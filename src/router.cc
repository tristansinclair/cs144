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
  // Iterate over all network interfaces in the router
  for ( auto& interface : _interfaces ) {
    auto& datagramQueue = interface->datagrams_received(); // Queue of received datagrams for the current interface

    // Process each datagram in the queue
    while ( !datagramQueue.empty() ) {
      auto& curDatagram = datagramQueue.front();

      // Drop the datagram if its TTL is 1 or less, ensuring it's not forwarded with a TTL of 0 or less
      if ( curDatagram.header.ttl <= 1 ) {
        datagramQueue.pop();
        continue;
      }

      // Decrement the TTL for the datagram to be forwarded
      curDatagram.header.ttl--;

      uint8_t bestPrefixLength = 0;
      RouteRule* bestRoute = nullptr;

      // Iterate through all routing rules to find the best match based on the longest prefix match
      for ( RouteRule& rule : _route_rules ) {

        if ( rule.prefix_length >= bestPrefixLength ) {

          if ( rule.prefix_length == 0 ) {
            bestPrefixLength = rule.prefix_length;
            bestRoute = &rule;
            continue;
          }

          uint32_t mask
            = 0xFFFFFFFF << ( 32 - rule.prefix_length ); // Create a bitmask for the current rule's prefix length

          // Compare the high-order bits of the datagram's destination and the rule's prefix using the bitmask
          if ( ( rule.route_prefix & mask ) == ( curDatagram.header.dst & mask ) ) {
            // Update the best match if this rule has a longer prefix than previously found
            if ( rule.prefix_length > bestPrefixLength ) {
              bestPrefixLength = rule.prefix_length;
              bestRoute = &rule;
            }
          }
        }
      }

      // If a matching route is found, forward the datagram accordingly
      if ( bestRoute ) {
        Address next_hop = bestRoute->next_hop.has_value()
                             ? bestRoute->next_hop.value()
                             : Address::from_ipv4_numeric(
                               curDatagram.header.dst ); // Use the final destination if no next hop is specified
        // Forward the datagram through the designated interface
        _interfaces[bestRoute->interface_num]->send_datagram( curDatagram, next_hop );
      }

      // Remove the processed datagram from the queue
      datagramQueue.pop();
    }
  }
}