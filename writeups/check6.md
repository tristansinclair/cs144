Checkpoint 6 Writeup
====================

My name: Tristan Sinclair

My SUNet ID: tristans

I collaborated with: weigle23

I would like to thank/reward these classmates for their help: weigle23

This checkpoint took me about 4 hours to do. I did not attend the lab session.

**Program Structure and Design**
The router's design revolves around two primary functionalities encapsulated in the methods add_route and route. The data structure central to this design is a std::vector of RouteRule objects, _route_rules, where each RouteRule represents a routing rule with a network prefix, prefix length, optional next hop address, and interface number.

`add_route` Method: This function allows for the dynamic addition of routing rules to the router's routing table (_route_rules). Each routing rule specifies how packets destined for certain IP addresses should be handled, making the router's operation flexible and adaptable to changes in the network topology. The choice to make next_hop an std::optional<Address> provides the flexibility to directly route packets to attached networks by omitting the next-hop address.

`route` Method: This function processes each incoming datagram across all interfaces, determining the appropriate outgoing interface based on the longest prefix match against the routing table. The method's design ensures that datagrams with insufficient TTL are dropped, and those eligible for forwarding are sent to their next hop or destination. The use of a bitmask and bitwise operations for prefix matching is both efficient and straightforward, allowing for quick determination of the best route without resorting to more complex data structures like tries or hash tables.

Structure and Design of RouteRule: The RouteRule struct is designed to encapsulate all necessary information for making routing decisions. The use of std::optional for the next_hop field is a thoughtful design choice, accommodating directly connected routes where no next hop is needed. This design choice simplifies the route addition process and reduces the potential for errors in route specification.

**Implementation Challenges**
Implementing the longest prefix match logic efficiently was a challenge. The straightforward approach using bitwise operations and masks is effective but required careful consideration to ensure correctness, especially in edge cases where multiple rules could potentially match a datagram's destination address. Ensuring that the router correctly handles these scenarios without introducing performance bottlenecks was crucial.

**Remaining Bugs**
N/A

**Optional: I was surprised by:**
The efficiency of using bitwise operations for the longest prefix match was initially surprising. This method, while simple, proved to be both effective and efficient for the task at hand. It's a great example of how low-level operations can be leveraged to solve complex problems in networking, such as routing, with minimal overhead.