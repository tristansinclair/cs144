Checkpoint 1 Writeup
====================

My name: **Tristan Sinclair**

My SUNet ID: **tristans**

I collaborated with: **weigle23**

I would like to thank/reward these classmates for their help: **Weigle23**

This lab took me about **6 hours** to do. I **did not** attend the lab session.

**I was surprised by or edified to learn that:**

Implementing a reassembler has some weird edge cases that need to be thoruoghly thought about. Order for these edge cases also is extremely important as I found out through my debugging process.The edge cases such as pre-firstUnassembledIndex segments and empty strings gave me a lot of trouble.

**Describe Reassembler structure and design.**

The Reassembler is designed to reconstruct a byte stream from fragmented and potentially out-of-order segments. It uses a `string` for storing the reassembled data and a `vector<bool>` to track which bytes have been added. The design focuses on handling various cases like overlapping segments, gaps in the data, and managing the stream's capacity. Alternative designs could involve using more complex data structures like trees or heaps to manage segments more efficiently, but the current design balances simplicity and performance. The main strength of this design is its straightforward approach, making it easier to debug and understand. However, it's prone to issues with edge cases and may not scale well for very large or highly fragmented data streams.

**Implementation Challenges:**

1. **Infinite Loop with While Loop**: The initial implementation used a `while` loop to process and write data to the output stream. This approach proved problematic, particularly when erasing characters from `reassembledString_` and `charsAdded_`. The issue arose when the loop continued to run indefinitely, attempting to process and remove data beyond the stream's capacity. This infinite loop was a critical bug, highlighting the risk of using a loop condition based on mutable data structures without proper checks.

2. **Transition to For Loop for Batch Processing**: To resolve the infinite loop issue, the implementation was modified to use a `for` loop. This allowed for counting the number of bytes that could be written to the stream in one batch, rather than processing one byte at a time. This change not only fixed the infinite loop issue but also improved the efficiency of the code by reducing the number of writes to the output stream. Batch processing also simplified the logic for handling data segments and their corresponding status in `charsAdded_`.

3. **Handling the 'is_finished' Condition**: The original approach to determine if the data reassembly was complete (using an `is_finished` statement) was ineffective. To address this, the implementation was changed to use a `finalIndex_` calculation, providing a more reliable way to determine when all data had been processed. This change required careful consideration of the stream's indexing and the handling of out-of-order data segments.

4. **Dealing with Off-by-One Errors and Empty Strings**: A significant challenge was managing off-by-one errors, especially in string operations like `substr`. These errors often led to incorrect processing of data segments, particularly when handling edge cases such as empty strings. Resolving these issues involved a thorough review and adjustment of the indexing logic used throughout the implementation. Ensuring that all possible string lengths and positions were correctly handled required meticulous testing and debugging.

5. **Iterative Debugging Process**: The process of fixing one bug often led to the discovery of another. This iterative debugging was a challenging aspect, as it sometimes felt like going in circles. Each modification in the code had to be carefully evaluated to ensure it did not introduce new issues. This required a deep understanding of how different parts of the code interacted and affected each other.

6. **Code Optimization for Speed**: After resolving the functional issues, attention was turned to optimizing the code for speed. This involved reviewing the code to identify and eliminate any inefficiencies, such as unnecessary data copying or redundant operations. The aim was to enhance the performance of the reassembler, particularly in scenarios involving large or complex data streams.


**Remaining Bugs:**

N/A

- Optional: I had unexpected difficulty with: using the while loop to write data to the stream. I was getting infinite loops and I couldn't figure out why.