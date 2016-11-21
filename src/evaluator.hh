#ifndef EVALUATOR_HH
#define EVALUATOR_HH

#include <string>
#include <vector>
#include <chrono>

#include "random.hh"
#include "whiskertree.hh"
#include "fintree.hh"
#include "network.hh"
#include "problem.pb.h"
#include "answer.pb.h"
#include "network.hh"

struct Statistics
{
  double always_on_10_score;
  double always_on_50_score;
  double always_on_100_score;
  double always_on_10_queue;
  double always_on_50_queue;
  double always_on_100_queue;
  double always_on_queue_tick;

  double regular_10_score;
  double regular_50_score;
  double regular_100_score;
  double regular_10_queue;
  double regular_50_queue;
  double regular_100_queue;
  double regular_queue_tick;
};

template <typename T>
class Evaluator
{
public:
  class Outcome
  {
  public:
    double score;
    std::chrono::milliseconds time;
    std::vector< std::pair< NetConfig, std::vector< std::pair< double, double > > > > throughputs_delays;
    T used_actions;
    Statistics statistics = {};
    Outcome() : score( 0 ), time( 0 ), throughputs_delays(), used_actions(), statistics() {
    }

    Outcome( const AnswerBuffers::Outcome & dna );

    AnswerBuffers::Outcome DNA( void ) const;
  };

private:
  const unsigned int _prng_seed;
  unsigned int _tick_count;

  std::vector< NetConfig > _configs;

  ProblemBuffers::Problem _ProblemSettings_DNA ( void ) const;

public:
  Evaluator( const ConfigRange & range );
  
  ProblemBuffers::Problem DNA( const T & actions ) const;

  Outcome score( T & run_actions,
		const bool trace = false,
		const double carefulness = 1) const;
  Outcome evaluate_for_bailout( T & run_actions,
    const bool trace = false,
    const double carefulness = 1, const bool always_on = false) const;
  static Evaluator::Outcome parse_problem_and_evaluate( const ProblemBuffers::Problem & problem );
  static Outcome score( T & run_actions,
			const unsigned int prng_seed,
			const std::vector<NetConfig> & configs,
			const bool trace,
			const unsigned int ticks_to_run );
  static Outcome evaluate_for_bailout( T & run_actions,
      const unsigned int prng_seed,
      const std::vector<NetConfig> & configs,
      const bool trace,
      const unsigned int ticks_to_run, const bool always_on );
};

#endif
