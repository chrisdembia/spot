#pragma once

#include "objective.h"
#include "reporter.h"

#include "flut/prop_node.hpp"
#include "flut/system/types.hpp"

#include "flut/math/math.hpp"
#include "flut/math/optional.hpp"
#include "flut/system_tools.hpp"
#include "flut/circular_deque.hpp"

#include <atomic>
#include <functional>
#include <thread>
#include "stop_condition.h"

#if defined(_MSC_VER)
#	pragma warning( push )
#	pragma warning( disable: 4251 )
#endif

namespace spot
{
	class SPOT_API optimizer
	{
	public:
		optimizer( const objective& o, const prop_node& pn = prop_node() );
		virtual ~optimizer();

		const stop_condition* step();
		const stop_condition* run( size_t number_of_steps = 0 );
		void run_threaded(); // TODO: use future / promise

		void add_stop_condition( s_ptr< stop_condition > cb ) { stop_conditions_.emplace_back( std::move( cb ) ); }
		stop_condition* current_stop_condition() const { return stop_condition_; }
		bool is_active() const { return stop_condition_ == nullptr; }
		template< typename T > T* find_stop_condition() const
		{ for ( auto& sc : stop_conditions_ ) { T* p = dynamic_cast< T* >( sc.get() ); if ( p ) return p; } return nullptr; }

		void add_reporter( s_ptr< reporter > cb ) { reporters_.emplace_back( std::move( cb ) ); }

		void signal_abort() { abort_flag_ = true; }
		void abort_and_wait();
		bool test_abort() const { return abort_flag_; }

		fitness_vec_t evaluate( const search_point_vec& pop );
		
		void set_max_threads( int val ) { max_threads = val; }

		int current_step() const { return step_count_; }
		fitness_t current_step_median() const { return current_step_median_; }
		fitness_t current_step_average() const { return current_step_average_; }
		fitness_t current_step_best() const { return current_step_best_; }
		const search_point& current_step_best_point() const { return current_step_best_point_; }

		fitness_t best_fitness() const { return best_fitness_; }
		const search_point& best_point() const { return best_point_; }

		const objective_info& info() const { return objective_.info(); }
		const objective& obj() const { return objective_; }
		bool is_better( fitness_t a, fitness_t b ) const { return objective_.info().is_better( a, b ); }

		// state
		virtual void save_state( const path& filename ) const { FLUT_NOT_IMPLEMENTED; }
		virtual objective_info make_updated_objective_info() const { FLUT_NOT_IMPLEMENTED; }

		// properties
		int max_threads = 1;

	protected:
		virtual void internal_step() = 0;

		// evaluation settings
		std::atomic_bool abort_flag_ = false;

		std::thread background_thread;
		flut::thread_priority thread_priority;
		stop_condition* stop_condition_;

		int step_count_;
		fitness_t current_step_median_;
		fitness_t current_step_average_;
		fitness_t current_step_best_;
		search_point current_step_best_point_;

		fitness_t best_fitness_;
		search_point best_point_;

		const objective& objective_;
		vector< s_ptr< reporter > > reporters_;
		vector< s_ptr< stop_condition > > stop_conditions_;
	};
}

#if defined(_MSC_VER)
#	pragma warning( pop )
#endif
