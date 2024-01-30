Checkpoint 2 Writeup
====================

**My name:** Tristan Sinclair

**My SUNet ID:** tristans

**I collaborated with:**

**I would like to thank/reward these classmates for their help:**

This lab took me about 6 hours to do. I did not attend the lab session.

**Describe Wrap32 and TCPReceiver structure and design.** 

`Wrap32` is a class that takes a 32 bit unsigned int and allows it to properly overflow past their capacity and back around to 0. This class is super useful because when receiving packets, the sequence number that comes with the packet is a 32 bit unsigned int, and there are often more packets than the capacity of a 32 bit unsigned int. Furthermore the initial packet, the SYN packet, has a randomized ISN, making the possibility of overflow even more likely. With many packets being sent, it would be interesting to see how well a 64 bit unsigned int would work, but even then still with the ISN being randomized, it would be possible to overflow. This could be a design approach but it still would not affect the overall design of the program or the speed I believe. 

`TCPReceiver` is a class which build on top of our reassembler in order to ensure that packets are being received properly, with a 2 way communication channel open in order to ensure that the sender 

The `receive` method checks if the incoming message has the `RST` flag set, and if so, it sets an error flag in the reader. If the `SYN` flag is set, it sets the `zero_point_` and inserts the payload into the Reassembler at index 0. It then converts the sequence number to an index for the Reassembler and inserts the payload at the correct position.

The send method in `TCPReceiver` class constructs a `TCPReceiverMessage` based on the receiver's state. If an error is detected in the Reassembler's reader, it returns a message with the `RST` flag set. Otherwise, it prepares a message with the acknowledgment number set to the next expected byte sequence number, adjusted for `SYN` and `FIN` flags. The window size is set to the maximum of `UINT16_MAX` and the available capacity of the writer, indicating the receiver's readiness to accept more bytes. This message helps the sender adjust its sending rate and detect any receiver-side errors.

**Implementation Challenges:**

I think the design of TCP is interesting, especially the fact that the ISN is randomized making the math a bit hard to conprehend at first. It took me lot's of drawings and simple examples to think thorugh the wrapping and the math. This especially was tough for the `unwrap` function. After some thoguht and some drawings I was able to figure it out. 

The TCPReceiver design took me a good deal of time. Even with drawing out diagrams and trying to understand the math, it was hard to understand how it exactly fit together, specifically the number of capacity and the different bytes added such as SYN and FIN. I had a ton of off by 1 issues and at a certain point was really just trying to get it to past tests.

For the `receive()` function, I first did some check to look at the different flags, and whether or not the zero_point for the TCP class had been set or not. This was important because we needed to ignore all messages until we got the zero point and knew where the beginning really was. Otherwise with the wrapped 32 bit uint, we really wouldn't know where the beginning was and what packet we were receiving. We also needed to communicate with the sender to let them know where we were and what we needed with the `TCPReceiver::send()` function. This is an intersting point in which maybe some improvement could've been made to my code, but I'm not sure if there is a better way. We really just ignored everything till we got the start flag.

Once receiving the ISN, we could then begin calculaitng the `firstUnassembledIndex` in order to insert the data into the `reassembler`. This is something that was tough because I didnt understand how to do this well. One of the approaches I thought of was to maybe use another private variable in the class to keep track of the `firstUnassembledIndex` but this was a bad idea given the packet randomness. It could have maybe been a alternate approach, but I moved away from it quickly. 

Finally when sending a message back to the sender, I first checked for errors, then worked to build the `ackno` message and the `window_size`. This was one of the hardest part because I wasn't sure how to calculate these values. The `window_size` I was able to figure out after some testing because it was the first fail I was running into after cleaning up some `std::optional` errors. The `ackno` message was really tough and I did not understand why I was getting errors. After reading on ed and looking through tests, I was able to figure out some of the off by one errors I was getting, in which I needed to add 1 if the message was a FIN message.


**Remaining Bugs:**
N/A


- Optional: I'm not sure about: 
  - The part I had the hardest time was with figuring out how to use the `bytes_pushed()` vs. `available_capacity()` and those `writer()` functions. It didn't make a ton of sense to me even after drawing it up, and I really was experimenting with code and trying to pass tests rather than understanding the design.
