#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  const uint64_t available_capacity = writer().available_capacity();
  // available_capacity is the number of bytes we can store in the reassembler

  // when the available capacity changes, it means data from the stream has been popped
  if ( available_capacity != charsAdded_.capacity() ) {
    charsAdded_.resize( available_capacity );
    reassembledString_.resize( available_capacity );
  }

  // if the first index is greater than the length of lastPushedIndex_ + available_capacity,
  // then return (we can't store this data)
  // or if the first index is less than the lastPushedIndex_, then return (we can't store this data)
  // TODO overlap case???? after or
  if ( first_index > lastPushedIndex_ + available_capacity ) {
    return;
  }
  if ( first_index < lastPushedIndex_ ) {
    return;
  }

  // using the first index and the data, push this substring onto the reassembledString_
  // trim if the data is longer than the available capacity
  const uint64_t startIndex = first_index - lastPushedIndex_; // the index of the string
  const uint64_t endIndex = min( startIndex + data.length(), available_capacity );
  // now insert the data into the reassembledString_
  reassembledString_.replace( startIndex, endIndex, data.substr( 0, endIndex - startIndex ) );
  // now also update the charsAdded_ vector
  for ( uint64_t i = startIndex; i < endIndex; i++ ) {
    charsAdded_[i] = true;
  }

  // now we need to check if we can write to the output stream using the charsAdded_ vector
  // if the first index is true, then we can write to the output stream until we hit a false
  while ( charsAdded_[0] ) {
    // now we can write to the output stream
    output_.writer().push( reassembledString_.substr( 0, 1 ) );
    // now we need to remove the first character from the reassembledString_ and charsAdded_
    reassembledString_.erase( 0, 1 );
    charsAdded_.erase( charsAdded_.begin() );
    // now we need to update the lastPushedIndex_
    lastPushedIndex_++;
  }

  // now we need to check if the stream is finished
  if ( is_last_substring ) {
    // if the stream is finished, then we need to close the output stream
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
