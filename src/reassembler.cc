#include "reassembler.hh"
#include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{

  const uint64_t availableCapacity = writer().available_capacity();
  uint64_t firstUnassembledIndex = reader().bytes_popped() + reader().bytes_buffered();

  if ( is_last_substring ) {
    finalIndex_ = first_index + data.length();
    seenFinalIndex_ = true;
  }

  // when the available capacity changes, it means data from the stream has been popped
  if ( availableCapacity != charsAdded_.capacity() ) {
    charsAdded_.resize( availableCapacity );
    reassembledString_.resize( availableCapacity );
  }

  // if the first index is past the available capacity, then we can't store this substring
  if ( first_index >= firstUnassembledIndex + availableCapacity ) {
    return;
  }
  if ( first_index < firstUnassembledIndex ) {
    if ( first_index + data.length() - 1 >= firstUnassembledIndex ) {
      // if the first index is before the firstUnassembledIndex, then we need to trim the data
      // to only include the part that we can store

      data = data.substr( firstUnassembledIndex - first_index );
      first_index = firstUnassembledIndex;

    } else {
      return;
    }
  }

  // using the first index and the data, push this substring onto the reassembledString_
  // trim if the data is longer than the available capacity
  const size_t startIndex = first_index - firstUnassembledIndex; // the index of the string
  const size_t endIndex = min( startIndex + data.length(), startIndex + availableCapacity );

  // now insert the data into the reassembledString_
  reassembledString_.replace( startIndex, endIndex, data.substr( 0, endIndex - startIndex ) );

  for ( size_t i = startIndex; i < endIndex; i++ ) {
    charsAdded_[i] = true;
  }

  size_t bytesToWrite = 0;
  for ( size_t i = 0; i < availableCapacity; i++ ) {
    if ( !charsAdded_[i] ) {
      // bytesToWrite = i;
      break;
    }
    bytesToWrite++;
  }

  if ( bytesToWrite > 0 ) {
    output_.writer().push( reassembledString_.substr( 0, bytesToWrite ) );
    // lastPushedIndex_ += bytesToWrite;

    reassembledString_.erase( 0, bytesToWrite );
    charsAdded_.erase( charsAdded_.begin(), charsAdded_.begin() + bytesToWrite );
  }

  firstUnassembledIndex += bytesToWrite;

  if ( finalIndex_ == firstUnassembledIndex ) {
    output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t numTrue = 0;
  for ( uint64_t i = 0; i < charsAdded_.size(); i++ ) {
    if ( charsAdded_[i] ) {
      numTrue++;
    }
  }
  return numTrue;
}
