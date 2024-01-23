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

  // if the first index is past the available capacity, then we can't store this substring
  if ( first_index > lastPushedIndex_ + available_capacity ) {
    return;
  }

  // using the first index and the data, push this substring onto the reassembledString_
  // trim if the data is longer than the available capacity
  const size_t startIndex = first_index - lastPushedIndex_; // the index of the string
  const size_t endIndex = min( startIndex + data.length(), available_capacity );

  // hadnle the case where the data is shorter than the available capacity, but it is the last substring
  bool unable_to_store = false;
  if ( is_last_substring && endIndex < available_capacity ) {
    unable_to_store = true;
  }

  // now insert the data into the reassembledString_
  size_t insertLength = min( data.length(), available_capacity - startIndex );
  reassembledString_.replace( startIndex, insertLength, data, 0, insertLength );

  // Update charsAdded_ vector
  for ( size_t i = startIndex; i < startIndex + insertLength; i++ ) {
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
  if ( is_last_substring && !unable_to_store ) {
    // if the stream is finished, then we need to close the output stream

    // check if there are any true bytes left in the charsAdded_ vector
    bool allFalse = true;
    for ( size_t i = 0; i < charsAdded_.size(); i++ ) {
      if ( charsAdded_[i] ) {
        allFalse = false;
        break;
      }
    }

    // if there are no true bytes left, then we can close the output stream
    if ( allFalse ) {
      output_.writer().close();
    }
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
