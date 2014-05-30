#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "configrange.hh"
#include "evaluator.hh"
#include "network.cc"
#include "rat-templates.cc"

Evaluator::Evaluator( const WhiskerTree & s_whiskers, const ConfigRange & range )
  : _prng( global_PRNG()() ), /* freeze the PRNG seed for the life of this Evaluator */
    _whiskers( s_whiskers ),
    _configs()
{
  double link_speed = range.link_packets_per_ms.first;

  /* this approach only varies muxing */
  assert( range.rtt_ms.first == range.rtt_ms.second );
  assert( range.link_packets_per_ms.first == range.link_packets_per_ms.second );

  for ( uint32_t i = 1; i <= 100 ; i = i + 1 ) {
    _configs.push_back( NetConfig().set_link_ppt( link_speed ).set_delay( range.rtt_ms.first ).set_num_senders( i ).set_on_duration( range.mean_on_duration ).set_off_duration( range.mean_off_duration ) );
  }
}

Evaluator::Outcome Evaluator::score( WhiskerTree & run_whiskers,
				     const bool trace, const unsigned int carefulness ) const
{
  PRNG run_prng( _prng );

  run_whiskers.reset_counts();

  /* run tests */
  Outcome the_outcome;
  for ( auto &x : _configs ) {
    const double dynamic_tick_count = 10000.0 * x.num_senders;

    /* run once */
    Network<Rat> network1( Rat( run_whiskers, trace ), run_prng, x );
    network1.run_simulation( dynamic_tick_count * carefulness );

    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }

  the_outcome.used_whiskers = run_whiskers;

  return the_outcome;
}

Evaluator::Outcome Evaluator::score( const std::vector< Whisker > & replacements,
				     const bool trace, const unsigned int carefulness ) const
{
  PRNG run_prng( _prng );

  WhiskerTree run_whiskers( _whiskers );
  for ( const auto &x : replacements ) {
    assert( run_whiskers.replace( x ) );
  }

  return score( run_whiskers, trace, carefulness );
}
