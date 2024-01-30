#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // zero_point is the Initial Sequence Number (ISN) of the connection
  // absolute checkpoint sequence number is a number in the ballpark of the desired answer

  // using the zero_point and the checkpoint, we can calculate the number of wraps
  // and then find the absolute sequence number that wraps to this Wrap32
  // that is closest to the checkpoint

  // helper functions
  // Wrap32 operator+( uint32_t n ) const { return Wrap32 { raw_value_ + n }; }
  // bool operator==( const Wrap32& other ) const { return raw_value_ == other.raw_value_; }

  uint32_t offset = raw_value_ - wrap( checkpoint, zero_point ).raw_value_;
  uint64_t result = checkpoint + offset;

  if ( offset > ( 1u << 31 ) && result >= ( 1ull << 32 ) ) {
    result -= ( 1ull << 32 );
  }

  return result;
}