#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // zero_point is the Initial Sequence Number (ISN) of the connection
  // absolute checkpoint sequence number 

  // helper functions
  // Wrap32 operator+( uint32_t n ) const { return Wrap32 { raw_value_ + n }; }
  // bool operator==( const Wrap32& other ) const { return raw_value_ == other.raw_value_; }

}