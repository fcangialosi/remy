#include <cassert>
#include <limits>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>

#include "breeder.hh"

#include <chrono>

using namespace boost::accumulators;
using namespace std;

typedef accumulator_set< double, stats< tag::tail_quantile <boost::accumulators::right > > >
  accumulator_t_right;

template <typename T>
void Breeder< T >::apply_best_split( T & tree, const unsigned int generation ) const
{
  const Evaluator< T > eval( _options.config_range );
  auto outcome( eval.score( tree, true ) );

  while ( 1 ) {
    auto my_action( outcome.used_actions.most_used( generation ) );
    assert( my_action );

    T bisected_action( *my_action, true );

    if ( bisected_action.num_children() == 1 ) {
      printf( "Got unbisectable whisker! %s\n", my_action->str().c_str() );
      auto mutable_action ( *my_action );
      mutable_action.promote( generation + 1 );
      assert( outcome.used_actions.replace( mutable_action ) );
      continue;
    }

    printf( "Bisecting === %s === into === %s ===\n", my_action->str().c_str(), bisected_action.str().c_str() );
    assert( tree.replace( *my_action, bisected_action ) );
    break;
  }
}

template <typename T, typename A>
ActionImprover< T, A >::ActionImprover( const Evaluator< T > & s_evaluator,
				  const T & tree,
				  const double score_to_beat)
  : eval_( s_evaluator ),
    tree_( tree ),
    score_to_beat_( score_to_beat )
{}

template <typename T, typename A>
void ActionImprover< T, A >::evaluate_replacements(const vector<A> &replacements,
    vector< pair< const A &, future< tuple< bool, double, unsigned long long > > > > &scores,
    const double carefulness ) 
{
  for ( const auto & test_replacement : replacements ) {
    if ( eval_cache_.find( test_replacement ) == eval_cache_.end() ) {
      /* need to fire off a new thread to evaluate */
      scores.emplace_back( test_replacement,
                           async( launch::async, [] ( const Evaluator< T > & e,
                                                      const A & r,
                                                      const T & tree,
                                                      const double carefulness ) {
                                    T replaced_tree( tree );
                                    const bool found_replacement __attribute((unused)) = replaced_tree.replace( r );
                                    assert( found_replacement );
																		auto result = e.score( replaced_tree, false, carefulness );
                                    return make_tuple( true, result.score, (unsigned long long)result.time_elapsed.count() ); },
                                  eval_, test_replacement, tree_, carefulness ) );
    } else {
      /* we already know the score */
      scores.emplace_back( test_replacement,
        async( launch::deferred, [] ( const double value ) {
               return make_tuple( false, value, ((unsigned long long)-1) ); }, eval_cache_.at( test_replacement ) ) );
    }
  } 
}

template <typename T, typename A>
void ActionImprover< T, A >::find_bad_replacements(const vector<A> &replacements,
    vector< pair< const A &, future< pair< bool, bool > > > > &scores,
    const double carefulness ) 
{
  for ( const auto & test_replacement : replacements ) {
    /* need to fire off a new thread to evaluate */
    scores.emplace_back( test_replacement,
                         async( launch::async, [] ( const Evaluator< T > & e,
                                                    const A & r,
                                                    const T & tree,
                                                    const double carefulness ) {
                                  T replaced_tree( tree );
                                  const bool found_replacement __attribute((unused)) = replaced_tree.replace( r );
                                  assert( found_replacement );
                                  return make_pair( true, e.evaluate_in_isolation( replaced_tree, false, carefulness ) ); },
                                eval_, test_replacement, tree_, carefulness ) );
  } 
}

/*

template <typename T, typename A>
vector<A> ActionImprover< T, A >::early_bail_out( const vector< A > &replacements,
        const double carefulness, const double quantile_to_keep )
{  
  vector< pair< const A &, future< pair< bool, double > > > > scores;
  evaluate_replacements( replacements, scores, carefulness );
  
  accumulator_t_right acc(
     tag::tail< boost::accumulators::right >::cache_size = scores.size() );
  vector<double> raw_scores;
  for ( auto & x : scores ) {
    const double score( x.second.get().second );
    acc( score );
    raw_scores.push_back( score );
  }
  
  double lower_bound = std::min( score_to_beat_ * (1 + MAX_PERCENT_ERROR), 
        score_to_beat_ * (1 - MAX_PERCENT_ERROR) );
  double quantile_bound = quantile( acc, quantile_probability = 1 - quantile_to_keep );
  double cutoff = std::max( lower_bound, quantile_bound );

  vector<A> top_replacements;
  for ( uint i = 0; i < scores.size(); i ++ ) {
    const A & replacement( scores.at( i ).first );
    const double score( raw_scores.at( i ) );
    if ( score >= cutoff ) {
      top_replacements.push_back( replacement );
    }
  }
  return top_replacements;
}
*/

template <typename T, typename A>
vector<A> ActionImprover< T, A >::early_bail_out_queue( const vector< A > &replacements,
        const double carefulness)
{  
  vector< pair< const A &, future< pair< bool, bool > > > > scores;
  find_bad_replacements( replacements, scores, carefulness );
  
  accumulator_t_right acc(
     tag::tail< boost::accumulators::right >::cache_size = scores.size() );
  vector<bool> raw_scores;
  for ( auto & x : scores ) {
    const bool is_bad( x.second.get().second );
    acc( is_bad );
    raw_scores.push_back( is_bad );
  }
  
  /* Discard replacements below threshold */
  vector<A> top_replacements;
  for ( uint i = 0; i < scores.size(); i ++ ) {
    const A & replacement( scores.at( i ).first );
    const bool is_bad ( raw_scores.at( i ) );
    if ( !is_bad ) {
      top_replacements.push_back( replacement );
    }
  }
  return top_replacements;
}

template <typename T, typename A>
double ActionImprover< T, A >::improve( A & action_to_improve )
{
	cout << "<<<improve>>>" << endl;
  auto replacements = get_replacements( action_to_improve );
  vector< pair< const A &, future< tuple< bool, double, unsigned long long > > > > scores;

  /* Run for 10% simulation time in isolation (1 sender always on) to see 
   * how sender fills up the router buffer and discard those that are too
   * passive or too aggressive early on */
  vector<A> top_replacements = early_bail_out_queue( replacements, 1.0 );
	unsigned int num_removed = (replacements.size() - top_replacements.size());
	if (num_removed > 0) {
		cout << ">>> REMOVED=" << num_removed << endl;
	}

	unsigned long long maxll = ((unsigned long long)-1);

  /* find best replacement */
  evaluate_replacements( top_replacements, scores, 1 );
  for ( auto & x : scores ) {
     const A & replacement( x.first );
     const auto outcome( x.second.get() );
     const bool was_new_evaluation( std::get<0>(outcome) );
     const double score( std::get<1>(outcome) );
		 const unsigned long long time_elapsed( std::get<2>(outcome) );

		 if (time_elapsed != (maxll)) {
		   cout << "<<< " << time_elapsed << endl;
		 } 
     /* should we cache this result? */
     if ( was_new_evaluation ) {
       eval_cache_.insert( make_pair( replacement, score ) );
     }

     if ( score > score_to_beat_ ) {
      score_to_beat_ = score;
      action_to_improve = replacement;
     }
  }

  cout << "Chose " << action_to_improve.str() << endl;

  return score_to_beat_;
}

template class ActionImprover< WhiskerTree, Whisker >;
template class ActionImprover< FinTree, Fin >;
template class Breeder< FinTree >;
template class Breeder< WhiskerTree >;
