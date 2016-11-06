#ifndef UTILITY_HH
#define UTILITY_HH

#include <cmath>
#include <cassert>
#include <climits>
#include "simulationresults.pb.h"

class Utility
{
private:
  double _tick_share_sending;
  unsigned int _packets_received;
  double _total_delay;
  int _last_seq_num;
  double _loss_rate;
public:
  Utility( void ) : _tick_share_sending( 0 ), _packets_received( 0 ), _total_delay( 0 ), _last_seq_num(-1), _loss_rate(0) {}

  void sending_duration( const double & duration, const unsigned int num_sending ) { _tick_share_sending += duration / double( num_sending ); }
  void packets_received( const std::vector< Packet > & packets ) {
    _packets_received += packets.size();

    for ( auto &x : packets ) {
      _last_seq_num = x.seq_num;
      assert( x.tick_received >= x.tick_sent );
      _total_delay += x.tick_received - x.tick_sent;
    }
    _loss_rate = _packets_received/(_last_seq_num + 1); // not completely accurate as the last packet may be lost
  }

  /* returns throughput normalized to equal share of link */
  double average_throughput_normalized_to_equal_share( void ) const
  {
    if ( _tick_share_sending == 0 ) {
      return 0.0;
    }
    return double( _packets_received ) / _tick_share_sending;
  }

  double average_delay( void ) const
  {
    if ( _packets_received == 0 ) {
      return 0.0;
    }
    return double( _total_delay ) / double( _packets_received );
  }

  double utility( void ) const
  {
    if ( _tick_share_sending == 0 ) {
      return 0.0;
    }

    if ( _packets_received == 0 ) {
      return -INT_MAX;
    }

    const double throughput_utility = log2( average_throughput_normalized_to_equal_share() );
    const double delay_penalty = log2( average_delay() / 100.0 );

    return throughput_utility - delay_penalty;
  }

  double packets_sent( void ) const
  {
    if ( _packets_received == 0 ) {
      return 0.0;
    }
    return _last_seq_num + 1;
  }

  double loss_rate( void ) const
  {
    if ( _packets_received == 0 ) {
      return 0.0;
    }
    return _loss_rate;
  }


  SimulationResultBuffers::UtilityData DNA() const {
    SimulationResultBuffers::UtilityData ret;
    ret.set_sending_duration( _tick_share_sending );
    ret.set_packets_received( _packets_received );
    ret.set_total_delay( _total_delay );
    return ret;
  }

};

#endif
