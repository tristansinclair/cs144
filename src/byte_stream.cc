#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  // handle the base cases
  if ( error_ || is_closed() || data.empty() ) {
    return;
  }

  // how many bytes to actually push
  const size_t bytesToPush = min( data.length(), available_capacity() );
  byteQueue_ += data.substr( 0, bytesToPush );
  bytesPushed_ += bytesToPush;

  return;
}

void Writer::close()
{
  closed_ = true;
  return;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - ( bytesPushed_ + bytesPopped_ );
}

uint64_t Writer::bytes_pushed() const
{
  return bytesPushed_;
}

bool Reader::is_finished() const
{
  return closed_ && byteQueue_.empty();
}

uint64_t Reader::bytes_popped() const
{
  return bytesPopped_;
}

string_view Reader::peek() const
{
  return byteQueue_;
}

void Reader::pop( uint64_t len )
{
  // handle the base cases
  if ( byteQueue_.empty() || len < 1 ) {
    return;
  }

  // gete the min of the len and the size of the queue
  if ( len > byteQueue_.size() ) {
    len = byteQueue_.size();
  }

  // pop the bytes off the queue
  // byteQueue_ = byteQueue_.substr( 0, len );
  byteQueue_.erase( 0, len );
  bytesPopped_ += len;

  return;
}

uint64_t Reader::bytes_buffered() const
{
  return bytesPushed_ - bytesPopped_;
}
