Checkpoint 0 Writeup
====================

My name: Tristan Sinclair
My SUNet ID: tristans

I collaborated with: Weigle23

I would like to credit/thank these classmates for their help: Weigle23

This lab took me about 5 hours to do. I did not attend the lab session.

My secret code from section 2.1 was: 560838

I was surprised by or edified to learn that: the byte stream process is actually pretty simple.

Describe ByteStream implementation:
For the bytestream implementation, I added these state values:

```cpp
bool closed_ { false };
uint64_t bytesPushed_ { 0 };
uint64_t bytesPopped_ { 0 };
std::string byteQueue_ {};
```
These variables held necessary information for the bytestream. The closed_ variable was used to determine if the bytestream was closed or not. The bytesPushed_ and bytesPopped_ variables were used to keep track of the number of bytes pushed and popped from the bytestream. The byteQueue_ variable was used to hold the bytes that were pushed into the bytestream.

I think this is a pretty simple and basic design, but because of its simplicity, it could cause problems. I think there are also risks if the stream were to be cutoff in middle of one of these functions. I wasn't able to test this but it could be an issue. I thought this homework was pretty straightforward and I'm excited to see what we do next!