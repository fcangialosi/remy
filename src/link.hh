#ifndef LINK_HH
#define LINK_HH

#include <deque>
#include <vector>

#include "packet.hh"
#include "delay.hh"

class Link
{
private:
  std::deque< Packet > _buffer;

  Delay _pending_packet;

  unsigned int _limit;
  unsigned int _largest_queue;
  bool _debug;
public:
  Link( const double s_rate,
	const unsigned int s_limit )
    : _buffer(), _pending_packet( 1.0 / s_rate ), _limit( s_limit ), _largest_queue( 0 ), _debug(false) {}

  void accept( const Packet & p, const double & tickno ) noexcept {
    if ( _pending_packet.empty() ) {
      _pending_packet.accept( p, tickno );
      if ( _debug )
        printf("In the accept function for delay for tickno accepting pending packet %f\n", tickno);
    } else {
      if ( _limit and _buffer.size() < _limit ) {
        _buffer.push_back( p );
        if ( (unsigned int)_buffer.size() > _largest_queue ) {
          if ( _debug )
            printf("THE BUFFER SIZE IS INCREASING FROM %u to %lu\n", _largest_queue, _buffer.size());
          _largest_queue = _buffer.size();
        }
      }
    }
  }

  template <class NextHop>
  void tick( NextHop & next, const double & tickno );

  double next_event_time( const double & tickno ) const { return _pending_packet.next_event_time( tickno ); }

  std::vector<unsigned int> packets_in_flight( const unsigned int num_senders ) const
  {
    std::vector<unsigned int> ret( num_senders );
    for ( const auto & x : _buffer ) {
      ret.at( x.src )++;
    }
    std::vector<unsigned int> propagating = _pending_packet.packets_in_flight( num_senders );
    for ( unsigned int i = 0; i < num_senders; i++ ) {
      ret.at( i ) += propagating.at( i );
    }
    return ret;
  }

  void set_rate( const double rate ) { _pending_packet.set_delay( 1.0 / rate ); }
  double rate( void ) const { return 1.0 / _pending_packet.delay(); }
  void set_limit( const unsigned int limit )
  {
    _limit = limit;
    while ( _buffer.size() > _limit ) {
      _buffer.pop_back();
    }
  }

  unsigned int get_largest_queue( void ) const
  {
    //printf("The largest queue is %u\n", _largest_queue);
    return _largest_queue;
  }
  unsigned int get_queue_size( void ) const
  {
    return (unsigned int)(_buffer.size());
  }
  void set_debug( const bool debug ) { _debug = debug; };
};

#endif
