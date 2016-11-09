#ifndef BREEDER_HH
#define BREEDER_HH

#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <future>
#include <vector>

#include "configrange.hh"
#include "evaluator.hh"

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

  std::unordered_map< A, double, boost::hash< A > > eval_cache_ {};

  double score_to_beat_;

  virtual std::vector< A > get_replacements( A & action_to_improve ) = 0;

  void evaluate_replacements(const std::vector< A > &replacements,
    std::vector< std::pair< const A &, std::future< std::tuple< bool, double, unsigned long long > > > > &scores,
    const double carefulness);

  void find_bad_replacements(const std::vector< A > &replacements,
    std::vector< std::pair< const A &, std::future< std::pair< bool, bool > > > > &scores,
    const double carefulness);

	/*
  std::vector< A > early_bail_out(const std::vector< A > &replacements,
        const double carefulness, const double quantile_to_keep);
*/
  std::vector< A > early_bail_out_queue(const std::vector< A > &replacements,
        const double carefulness);


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
