#include "reassembler.hh"
#include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Update firstUnassembledIndex at the start
  uint64_t firstUnassembledIndex = reader().bytes_popped() + reader().bytes_buffered();
  const uint64_t availableCapacity = writer().available_capacity();

  if ( is_last_substring ) {
    finalIndex_ = first_index + data.length();
  }

  // resize the vectors if necessary (after data from stream is popped)
  if ( availableCapacity != charsAdded_.capacity() ) {
    charsAdded_.resize( availableCapacity );
    reassembledString_.resize( availableCapacity );
  }

  // if the first index is past the available capacity, then we can't store this substring
  if ( first_index >= firstUnassembledIndex + availableCapacity ) {
    return;
  }

  // first_index is before the firstUnassembledIndex
  if ( first_index < firstUnassembledIndex ) {
    if ( first_index + data.length() <= firstUnassembledIndex ) {
      return; // data is entirely before the firstUnassembledIndex, ignore it
    }
    // trim data and adjust first_index
    data = data.substr( firstUnassembledIndex - first_index );
    first_index = firstUnassembledIndex;
  }

  // convert index to string/vector indexes for trimming
  size_t startIndex = first_index - firstUnassembledIndex;
  size_t endIndex = min( startIndex + data.length(), startIndex + availableCapacity );

  // fill in the data
  reassembledString_.replace( startIndex, endIndex - startIndex, data );
  fill( charsAdded_.begin() + startIndex, charsAdded_.begin() + endIndex, true );

  // get the number of bytes we can write, possibly 0!
  size_t bytesToWrite = 0;
  for ( size_t i = 0; i < availableCapacity; i++ ) {
    if ( !charsAdded_[i] ) {
      break;
    }
    bytesToWrite++;
  }

  // if we can write, then write to the stream and update the string/vector
  if ( bytesToWrite > 0 ) {
    output_.writer().push( reassembledString_.substr( 0, bytesToWrite ) );
    reassembledString_.erase( 0, bytesToWrite );
    charsAdded_.erase( charsAdded_.begin(), charsAdded_.begin() + bytesToWrite );
    firstUnassembledIndex += bytesToWrite;
  }

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
