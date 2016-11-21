#include "network.hh"
#include "simulationresults.hh"
#include "sendergangofgangs.cc"
#include "link-templates.cc"
#include "stochastic-loss.hh"
template <class Gang1Type, class Gang2Type>
Network<Gang1Type, Gang2Type>::Network( const typename Gang1Type::Sender & example_sender1,
					const typename Gang2Type::Sender & example_sender2,
					PRNG & s_prng,
					const NetConfig & config )
  : _prng( s_prng ),
    _senders( Gang1Type( config.mean_on_duration, config.mean_off_duration, config.num_senders, example_sender1, _prng ),
	      Gang2Type( config.mean_on_duration, config.mean_off_duration, config.num_senders, example_sender2, _prng, config.num_senders ) ),
    _link( config.link_ppt, config.buffer_size ),
    _delay( config.delay ),
    _rec(),
    _tickno( 0 ),
    _stochastic_loss( config.stochastic_loss_rate , _prng)
{
}

template <class Gang1Type, class Gang2Type>
Network<Gang1Type, Gang2Type>::Network( const typename Gang1Type::Sender & example_sender1,
					PRNG & s_prng,
					const NetConfig & config )
  : _prng( s_prng ),
    _senders( Gang1Type( config.mean_on_duration, config.mean_off_duration, config.num_senders, example_sender1, _prng ),
	      Gang2Type() ),
    _link( config.link_ppt, config.buffer_size ),
    _delay( config.delay ),
    _rec(),
    _tickno( 0 ),
    _stochastic_loss( config.stochastic_loss_rate , _prng)
{
}

template <class Gang1Type, class Gang2Type>
Network<Gang1Type, Gang2Type>::Network( const typename Gang1Type::Sender & example_sender1,
           PRNG & s_prng,
           const NetConfig & config,
           double num_senders)
  : _prng( s_prng ),
    _senders( Gang1Type( config.mean_on_duration, config.mean_off_duration, num_senders, example_sender1, _prng ),
      Gang2Type() ),
    _link( config.link_ppt, config.buffer_size ),
    _delay( config.delay ),
    _rec(),
    _tickno( 0 ),
    _stochastic_loss( config.stochastic_loss_rate , _prng)
{
}

template <class Gang1Type, class Gang2Type>
void Network<Gang1Type, Gang2Type>::tick( void )
{
  _senders.tick( _link, _rec, _tickno );
  _link.tick( _stochastic_loss, _tickno );
  _stochastic_loss.tick( _delay, _tickno );
  _delay.tick( _rec, _tickno );
}

template <class Gang1Type, class Gang2Type>
BailoutLogging Network<Gang1Type, Gang2Type>::run_simulation_bailout_logging( const double & duration, bool logging )
{
  // idea : run the simulation, but record what happens at 10 % of the time
  // send this back to the simulation to print some statistics

  BailoutLogging bailout;
  if ( logging )
    _link.set_debug(true);
  // TODO: run the regular sender for all the time
  // at 10 % of the evaluation time, stop and record the max queue size seen so far and the max size the queue has seen
  assert ( _tickno == 0 );
  bool early_written = false;
  bool mid_written = false;
  while ( _tickno < duration ) {
    /* find element with the soonest event */
    _tickno = min( min( _senders.next_event_time( _tickno ),
       min(_link.next_event_time( _tickno ), _stochastic_loss.next_event_time( _tickno)) ),
        min( _delay.next_event_time( _tickno ),
          _rec.next_event_time( _tickno ) ) );

     if ( (_tickno > .10 * duration) && !early_written ) {
       early_written = true;
       bailout.score_10 = _senders.utility();
       bailout.queue_10 = (double)(_link.get_largest_queue());
     }

     if ( (_tickno > .5 * duration) && !mid_written ) {
        mid_written = true;
        bailout.score_50 = _senders.utility();
        bailout.queue_50 = (double)(_link.get_largest_queue());
     }

    if ( _tickno > duration ) break;
    assert( _tickno < std::numeric_limits<double>::max() );

    tick();
  }

  bailout.score = _senders.utility();
  bailout.queue = (double)(_link.get_largest_queue());
  bailout.queue_tick = (_link.get_largest_queue_tick() / duration);

  return bailout;
}
template <class Gang1Type, class Gang2Type>
void Network<Gang1Type, Gang2Type>::run_simulation( const double & duration )
{
  assert( _tickno == 0 );

  while ( _tickno < duration ) {
    /* find element with soonest event */
    _tickno = min( min( _senders.next_event_time( _tickno ),
			min(_link.next_event_time( _tickno ), _stochastic_loss.next_event_time( _tickno)) ),
		   min( _delay.next_event_time( _tickno ),
			_rec.next_event_time( _tickno ) ) );

    if ( _tickno > duration ) break;
    assert( _tickno < std::numeric_limits<double>::max() );

    tick();
  }
}


template <class Gang1Type, class Gang2Type>
void Network<Gang1Type, Gang2Type>::run_simulation_until( const double tick_limit )
{
  if ( _tickno >= tick_limit ) {
    return;
  }

  while ( true ) {
    /* find element with soonest event */
    double next_tickno = min( min( _senders.next_event_time( _tickno ),
				   _link.next_event_time( _tickno ) ),
			      min( _delay.next_event_time( _tickno ),
				   _rec.next_event_time( _tickno ) ) );

    if ( next_tickno > tick_limit ) {
      _tickno = tick_limit;
      break;
    }

    assert( next_tickno < std::numeric_limits<double>::max() );

    _tickno = next_tickno;

    tick();
  }
}

template <class Gang1Type, class Gang2Type>
void Network<Gang1Type, Gang2Type>::run_simulation_with_logging_until( const double tick_limit,
                                                                       SimulationRunData & run_data,
                                                                       const double interval )
{
  if ( _tickno >= tick_limit ) {
    return;
  }

  double next_log_time = interval;

  while ( true ) {
    /* find element with soonest event */
    double next_tickno = min( min( _senders.next_event_time( _tickno ),
      _link.next_event_time( _tickno ) ),
       min( _delay.next_event_time( _tickno ),
      _rec.next_event_time( _tickno ) ) );

    if ( next_tickno > tick_limit ) {
      _tickno = tick_limit;
      break;
    }

    assert( next_tickno < std::numeric_limits<double>::max() );

    _tickno = next_tickno;
    tick();

    if ( _tickno > next_log_time ) {
      SimulationRunDataPoint & datum = run_data.add_datum( _tickno / 1000.0 );
      datum.add_sender_data( _senders.statistics_for_log() );
      datum.add_network_data( packets_in_flight() );
      next_log_time += interval;
    }

  }
}

template <class Gang1Type, class Gang2Type>
vector<unsigned int> Network<Gang1Type, Gang2Type>::packets_in_flight( void ) const
{
  unsigned int num_senders = _senders.count_senders();
  vector<unsigned int> ret = _link.packets_in_flight( num_senders );
  const vector<unsigned int> delayed = _delay.packets_in_flight( num_senders );

  for ( unsigned int i = 0; i < num_senders; i++ ) {
    ret.at( i ) += delayed.at( i );
  }

  return ret;
}
