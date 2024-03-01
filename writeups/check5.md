Checkpoint 5 Writeup
====================

My name: **Tristan Sinclair**

My SUNet ID: **tristans**

I collaborated with: **N/A**

I would like to thank/reward these classmates for their help: **weigle23**

This checkpoint took me about **4 hours** to do. I **did not** attend the lab session.

### Program Structure and Design

#### Implementation of Core Functions

- **send_datagram**: This function is responsible for sending out IP datagrams. It first checks if the destination's hardware address is known (i.e., present in the ARP table). If so, the datagram is encapsulated within an Ethernet frame and transmitted. If the address is unknown, an ARP request is issued (rate-limited by `arp_timer_table_`), and the datagram is queued (`waiting_for_arp_queue_`) until the address can be resolved. This design efficiently handles both known and unknown destination addresses, ensuring that datagrams are either sent immediately or queued for future transmission, optimizing network traffic and resource utilization.

- **recv_frame**: Upon receiving an Ethernet frame, this function first validates the destination address against the network interface's address and the broadcast address. It then distinguishes between IPv4 and ARP payloads, handling each accordingly. For ARP requests targeting the interface, it generates and sends an ARP reply. For ARP replies, it updates the ARP table and processes any queued datagrams awaiting this resolution. This approach ensures that incoming data is processed efficiently, ARP mappings are kept current, and queued datagrams are sent as soon as possible, enhancing the overall network performance.

- **tick**: This function updates the timers associated with ARP entries (`arp_timer_table_` and `arp_exp_timer_table_`). It purges expired ARP entries and adjusts rate-limiting counters, maintaining the integrity of the ARP table and ensuring that ARP requests are sent at appropriate intervals. This periodic maintenance is crucial for the stability and reliability of the network interface, ensuring that outdated information is removed and that the network interface adapts to changes in the network topology.

#### Data Structures for ARP Processing

The chosen data structures—`std::unordered_map` for ARP tables and `std::queue` for datagram queuing—offer efficient access and manipulation of networking data. These structures support the core functionalities of ARP processing, including address resolution, request rate-limiting, and management of pending transmissions, contributing to the robustness and efficiency of the network interface.

#### Alternative Designs and Considerations

One alternative design could involve leveraging a centralized ARP management system outside of individual network interfaces, potentially simplifying the network interface logic. However, this might introduce additional complexity and latency in address resolution. The current design, with direct ARP integration within each network interface, offers a good balance between efficiency, complexity, and modularity, allowing for quick address resolution and immediate handling of network traffic.

### Implementation Challenges

The implementation of the `send_datagram`, `recv_frame`, and `tick` functions, alongside the management of ARP-related data structures, presented several challenges. The main one was trying to understand the assignment sheet and figuring when you're actually suposed to retransmit messages.

Remaining Bugs:
**N/A**

