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

  uint32_t offset = raw_value_ - wrap( checkpoint, zero_point ).raw_value_; // take the difference between the two
  uint64_t result = checkpoint + offset;                                    // add the difference to the checkpoint

  // adjust for the closest of the two wrap points
  if ( offset > ( 1u << 31 ) && result >= ( 1ull << 32 ) ) {
    result -= ( 1ull << 32 );
  }

  return result;
}