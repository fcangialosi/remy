#ifndef BREEDER_HH
#define BREEDER_HH

#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <future>
#include <vector>
#include <chrono>
#include "configrange.hh"
#include "evaluator.hh"

struct OutcomeData
{
  double score;
  double score_10;
  double score_50;

  double always_on_score_10;
  double always_on_score_50;
  double always_on_score;

  double queue;
  double queue_10;
  double queue_50;
  double queue_tick;

  double always_on_queue_10;
  double always_on_queue_50;
  double always_on_queue;
  double always_on_queue_tick;
  float time;
  float always_on_time;

};
struct BreederOptions
{
  ConfigRange config_range = ConfigRange();
};

template <typename T, typename A>
class ActionImprover
{
protected:
  const double MAX_PERCENT_ERROR = 0.05;
  const Evaluator< T > eval_;

  T tree_;

  std::unordered_map< A, OutcomeData, boost::hash< A > > eval_cache_ {};
  std::unordered_map< A, OutcomeData, boost::hash< A > > eval_cache_early {};
  double score_to_beat_;

  virtual std::vector< A > get_replacements( A & action_to_improve ) = 0;
  void evaluate_for_bailout(const std::vector< A > &replacements,
      std::vector< std::pair< const A &, std::future< std::pair< bool, OutcomeData > > > > &scores,
      const double carefulness);
  /*void evaluate_replacements(const std::vector< A > &replacements,
    std::vector< std::pair< const A &, std::future< std::pair< bool, double > > > > &scores,
    const double carefulness);

  std::vector< A > early_bail_out(const std::vector< A > &replacements,
        const double carefulness, const double quantile_to_keep);*/

public:
  ActionImprover( const Evaluator<  T > & evaluator, const T & tree, 
                   const double score_to_beat );
  virtual ~ActionImprover() {};

  double improve( A & action_to_improve );
};

template <typename T>
class Breeder
{
protected:
  BreederOptions _options;

  void apply_best_split( T & tree, const unsigned int generation ) const;

public:
  Breeder( const BreederOptions & s_options ) : _options( s_options ) {};
  virtual ~Breeder() {};

  virtual typename Evaluator< T >::Outcome improve( T & whiskers ) = 0;
};

#endif
